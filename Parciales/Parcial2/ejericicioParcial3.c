/*
Programar el microcontrolador LPC1769 en un código de lenguaje C para que mediante
su ADC digitalice una señal analógica cuyo ancho de banda es de 16 Khz. 
La señal analógica tiene una amplitud de pico máxima positiva de 3.3 voltios.

Los datos deben ser guardados utilizando el Hardware GPDMA en la primera mitad de la
memoria SRAM ubicada en el bloque AHB SRAM - bank 0, de manera tal que permita almacenar
todos los datos posibles que esta memoria nos permita. Los datos deben ser almacenados
como un buffer circular conservando siempre las últimas muestras.

Por otro lado se tiene una forma de onda como se muestra en la imagen a continuación.
Esta señal debe ser generada por una función y debe ser reproducida por el DAC desde
la segunda mitad de AHB SRAM - bank 0 memoria utilizando DMA de tal forma que se logre
un periodo de 614us logrando la máxima resolución y máximo rango de tensión.

Durante operación normal se debe generar por el DAC la forma de onda mencionada como wave_form.
Se debe indicar cuál es el mínimo incremento de tensión de salida de esa forma de onda.

Cuando interrumpe una extint conectada a un pin, el ADC configurado debe completar el ciclo de
conversión que estaba realizando, y ser detenido, a continuación se comienzan a sacar las muestras
del ADC por el DAC utilizando DMA y desde las posiciones de memoria originales.

Cuando interrumpe nuevamente en el mismo pin, se vuelve a repetir la señal del DAC generada por
la forma de onda de wave_form previamente almacenada y se arranca de nuevo la conversión de datos del ADC.
Se alterna así entre los dos estados del sistema con cada interrupción externa.

Suponer una frecuencia de core clk de 80 Mhz. El código debe estar debidamente comentado.
*/

// SEÑAL DE 16KHZ ENTRA POR EL ADC
// LAS MUESTRAS DEL ADC SE GUARDAN EN LA PRIMERA MITAD DE LA SRAM BANK 0 MEDIANTE DMA, COMO BUFFER CIRCULAR
// SE TIENE UNA FORMA DE ONDA GUARDADA EN LA SEGUNDA MITAD DE LA SRAM BANK 0, QUE SE MUESTRA POR EL DAC MEDIANTE DMA, LOGRANDO UN PERIODO DE 614us
// DURANTE OPEARACION NORMAL SE MUESTRA LA FORMA DE ONDA POR EL DAC
// CUANDO HAY UNA INTERRUPCION EXTERNA, SE ESPERA QUE EL ADC TERMINE, SE DETIENE Y SE MUESTRAN LAS MUESTRAS DEL ADC POR EL DAC
// FRECUENCIA DE CORE CLK DE 80MHz

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_exti.h"

#define SRAM_BANK0_ADDR 0x2007C000
#define SRAM_BANK0_HALF_SIZE 0x2000 // 8KB = 8192 bytes
#define ADC_BUFFER_ADDR SRAM_BANK0_ADDR
#define DAC_BUFFER_ADDR (SRAM_BANK0_ADDR + SRAM_BANK0_HALF_SIZE)

#define BUFFER_SIZE (SRAM_BANK0_HALF_SIZE / sizeof(uint16_t))  // 4096 muestras de 16 bits

static uint32_t* wave_form = (uint32_t*)DAC_BUFFER_ADDR; // Puntero a la forma de onda en la segunda mitad de la SRAM_BANK0

// static uint16_t adc_sample[BUFFER_SIZE] = {0}; (No es necesario, se guarda directamente en la SRAM)

static volatile uint8_t mode = 0; // 0: DAC normal, 1: DAC con muestras del ADC

// Declaraciones de LLI como globales para poder acceder a ellas en las funciones de cambio de modo.
GPDMA_LLI_Type lli_adc, lli_dac_waveform, lli_dac_adc;

void configPin(void);
void configADC(void);
void configDMA(void);
void configDAC(void);
void configEXTI(void);
void EINT0_IRQHandler(void);
void ADC_IRQHandler(void);
void generate_wave_form(void);
void start_normal_mode(void);
void start_adc_playback_mode(void);

int main (void){
    // Suponiendo CCLK = 80MHz, PCLK_ADC = CCLK/4 = 20MHz
    // PCLK_DAC = CCLK/4 = 20MHz

    configPin();
    configADC();
    configDMA();
    configDAC();
    configEXTI();
    generate_wave_form();

    start_normal_mode(); // Empieza en modo normal mostrando la forma de onda por el DAC

    while(1);

    return 0;
}

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};

    //ADC
    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 23;
    pinCfg.Funcnum = 1;
    pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&pinCfg);

    //DAC
    pinCfg.Pinnum = 26;
    pinCfg.Funcnum = 2;
    PINSEL_ConfigPin(&pinCfg);

    //EXTI0
    pinCfg.Portnum = 2;
    pinCfg.Pinnum = 10;
    pinCfg.Funcnum = 1;
    pinCfg.Pinmode = PINSEL_PINMODE_PULLDOWN;
    PINSEL_ConfigPin(&pinCfg);
}

// CONFIGURACION DEL ADC
void configADC(void){
    ADC_Init(LPC_ADC, 100000);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE); // Habilito la interrupcion por canal 0 para cuando necesite hacer el cambio a playback
    // El canal del adc se habilita y deshabilita en las funciones de start_normal_mode y start_adc_playback_mode
    NVIC_SetPriority(ADC_IRQn, 1);
}

// CONFIGURACION DEL DAC
void configDAC(void){
    DAC_CONVERTER_CFG_Type dacCfg = {0};
    // Periodo de 614us con DAC_BUFFER_SIZE (4096) muestras.
    // F_update = 4096 / 614us = 6.67 MHz
    // Timeout = PCLK_DAC / F_update = 20MHz / 6.67MHz = 3
    // El valor de timeout es (PCLK / F_update) - 1, pero la API suma 1. Usamos 3.

    dacCfg.CNT_ENA = ENABLE;
    dacCfg.DBLBUF_ENA = DISABLE;
    dacCfg.DMA_ENA = ENABLE;

    DAC_Init(LPC_DAC);
    DAC_SetDMATimeOut(LPC_DAC, 3);
    DAC_ConfigDAConverterControl(LPC_DAC, &dacCfg);
   
}

// CONFIGURACION DE EXTI
void configEXTI(void){
    EXTI_InitTypeDef extiCfg = {0};

    extiCfg.EXTI_Line = 0;
    extiCfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    extiCfg.EXTI_polarity = EXTI_POLARITY_HIGH_ACTIVE_OR_RISING_EDGE;

    EXTI_Init();
    EXTI_Config(&extiCfg);
    NVIC_SetPriority(EINT0_IRQn, 0);
    NVIC_EnableIRQ(EINT0_IRQn);
}

// CONFIGURACION DE DMA
void configDMA(void){
    GPDMA_Init();

    // Lista donde se guardan las muestras del ADC
    lli_adc.SrcAddr = (uint32_t)&LPC_ADC -> ADGDR;
    lli_adc.DstAddr = (uint32_t)ADC_BUFFER_ADDR;
    lli_adc.NextLLI = (uint32_t)&lli_adc;
    lli_adc.Control = BUFFER_SIZE | (3 << 12) | (3 << 15) | (2 << 18) | (1 << 21) | (1 << 27);
    // SBSize y DBSize en 16 bits | SWidth en 32 bits y DWidth en 16 bits | DI en 1

    // Lista donde se guardan las muestras de la waveform
    lli_dac_waveform.SrcAddr = (uint32_t)DAC_BUFFER_ADDR;
    lli_dac_waveform.DstAddr = (uint32_t)&LPC_DAC -> DACR;
    lli_dac_waveform.NextLLI = (uint32_t)&lli_dac_waveform;
    lli_dac_waveform.Control = BUFFER_SIZE | (2 << 18) | (2 << 21) | (1 << 26);
    // SWidth y DWidth en 32 bits | SI en 1

    // lista donde se guardan las muestras del ADC para mostrarlas por el DAC
    lli_dac_adc.SrcAddr = (uint32_t)ADC_BUFFER_ADDR;
    lli_dac_adc.DstAddr = (uint32_t)&LPC_DAC -> DACR;
    lli_dac_adc.NextLLI = (uint32_t)&lli_dac_adc;
    lli_dac_adc.Control = BUFFER_SIZE | (1 << 18) | (2 << 21) | (1 << 26);
    // SWidth en 16 bits y DWidth en 32 bits | SI en 1

    GPDMA_Channel_CFG_Type adc_cfg = {0};

    // CANAL 0 DEL DMA PARA EL PRIMER MODO (ADC - SRAM)
    adc_cfg.ChannelNum = 0;
    adc_cfg.SrcMemAddr = 0;
    adc_cfg.DstMemAddr = ADC_BUFFER_ADDR;
    adc_cfg.TransferSize = BUFFER_SIZE;
    adc_cfg.TransferType = GPDMA_TRANSFERTYPE_P2M;
    adc_cfg.SrcConn = GPDMA_CONN_ADC;
    adc_cfg.DMALLI = (uint32_t)&lli_adc;
    GPDMA_Setup(&adc_cfg);

    // CANAL 1 DEL DMA PARA EL SEGUNDO MODO (SRAM - DAC)

    GPDMA_Channel_CFG_Type dac_waveform_cfg = {0};

    dac_waveform_cfg.ChannelNum = 1;
    dac_waveform_cfg.SrcMemAddr = DAC_BUFFER_ADDR;
    dac_waveform_cfg.TransferSize = BUFFER_SIZE;
    dac_waveform_cfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    dac_waveform_cfg.DstConn = GPDMA_CONN_DAC;
    dac_waveform_cfg.DMALLI = (uint32_t)&lli_dac_waveform;
    GPDMA_Setup(&dac_waveform_cfg);
}

// ------------------ HANDLERS ------------------

// HANDLER DE LA INTERRUPCION EXTERNA
void EINT0_IRQHandler(void){
    mode = !mode;

    if(mode){
        // Modo 1: Activo la interrupcion por ADC para que cuando termine la conversion se muestren las muestras por el DAC
        NVIC_EnableIRQ(ADC_IRQn);
    }else{
        // Modo 0: Muestro la waveform por el DAC y arranco el ADC
        start_normal_mode();
    }
    EXTI_ClearEXTIFlag(EXTI_EINT0);
}

// HANDLER DE LA INTERRUPCION DEL ADC
void ADC_IRQHandler(void){
    if(mode && ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE)){
        // Modo 1: Detengo el ADC y muestro las muestras por el DAC
        start_adc_playback_mode();
        NVIC_DisableIRQ(ADC_IRQn); // Deshabilito la interrupcion del ADC hasta que vuelva a modo normal
    }
}

// ------------------ FUNCIONES AUXILIARES ------------------

// FORMA DE ONDA A MOSTRAR POR EL DAC
void generate_wave_form(void){
    for(uint16_t i=0; i<BUFFER_SIZE; i++){
        uint32_t value = (i*1023)/(BUFFER_SIZE); // Rampa de 0 a 1023 (3.3V)
        wave_form[i] = value << 6; // El DAC usa 10 bits, los 6 LSB son ignorados
    }
}

// INICIA EL MODO NORMAL, MOSTRANDO LA FORMA DE ONDA POR EL DAC
void start_normal_mode(void){
    // Detener canales para reconfigurar de forma segura
    GPDMA_ChannelCmd(0, DISABLE);
    GPDMA_ChannelCmd(1, DISABLE);

    // Configurar canal 1 para mostrar la forma de onda por el DAC
    LPC_GPDMACH1->DMACCLLI = (uint32_t)&lli_dac_waveform; // Apunta a la lista de la waveform

    // Habilitar canales
    GPDMA_ChannelCmd(0, ENABLE); // ADC -> SRAM
    GPDMA_ChannelCmd(1, ENABLE); // SRAM (wave) -> DAC

    // Arrancar el ADC
    ADC_ChannelCmd(LPC_ADC, 0, ENABLE);
}

// INICIA EL MODO DE REPRODUCCION DE LAS MUESTRAS DEL ADC POR EL DAC
void start_adc_playback_mode(void){
    // Detener el adc
    ADC_ChannelCmd(LPC_ADC, 0, DISABLE);

    // Detener canales para reconfigurar de forma segura
    GPDMA_ChannelCmd(0, DISABLE);
    GPDMA_ChannelCmd(1, DISABLE);

    // Configurar canal 1 para mostrar las muestras del ADC por el DAC
    LPC_GPDMACH1 -> DMACCLLI = (uint32_t)&lli_dac_adc; // Apunta a la lista de las muestras del ADC

    GPDMA_ChannelCmd(1, ENABLE); // habilito solo el canal del DAC
}
