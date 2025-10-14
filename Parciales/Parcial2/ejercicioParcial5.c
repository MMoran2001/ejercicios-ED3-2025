/*
Mediante el DAC:
Se comienza a sacar las muestras del ADC por el DAC mediante el hardware GPDMA desde las posiciones
originales de memoria (DAC_DATA).
Se debe generar una función que represente la forma de onda de la figura, mediante 382 muestras, para ser
reproducida por el DAC desde la memoria SRAm ubicada en el bloque AHB SRAM-bank 1 utilizando el hardware
GPDMA de tal forma que se logre un periodo de 1Seg logrando la máxima resolución y máximo rango de tensión permitido.
(Se debe indicar cual es el mínimo incremento de tensión de salida de dicha forma de onda). (DAC_WAVE)

"Ver imagen de la forma de onda"
*/

/* Pense este ejercicio como si fueran dos modos que se cambian con un boton ubicado en EINT0, porque sino no se entiende como 
empieza a sacar las muestras del ADC por el DAC y despues cambia a la forma de onda, sin ninguna condicion.

Tambien podria hacer un cambio con timer cada x tiempo pero me fui a lo mas facil.
*/

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_exti.h"

#define DAC_WAVE_SIZE 382
#define SRAM_BANK1_ADDR 0x20080000 // Direccion de la SRAM bank 1
#define BUFFER_SIZE 1

static uint32_t dacTimeout = 65445;
static uint32_t adcFreq = 100000;
uint16_t* DAC_WAVE = (uint16_t*)SRAM_BANK1_ADDR; // Puntero a la forma de onda en la SRAM bank 1
static volatile uint8_t mode = 0; // 0 = ADC -> DAC | 1 = DAC_WAVE -> DAC

void configPin(void);
void configDAC(void);
void configADC(void);
void configDMA(void);
void configEXTI(void);
void EINT0_IRQHandler(void);
void generate_wave_form(void);

int main (){
    configPin();
    configDAC();
    configADC();
    configDMA();
    generate_wave_form();

    while(1);

    return 0;
}

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};

    // ADC
    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 23;
    pinCfg.Funcnum = 1;
    PINSEL_ConfigPin(&pinCfg);

    //DAC
    pinCfg.Pinnum = 26;
    pinCfg.Funcnum = 2;
    PINSEL_configPin(&pinCfg);

    //EXTI EINT0
    pinCfg.Portnum = 2;
    pinCfg.Pinnum = 10;
    pinCfg.Funcnum = 1;
    pinCfg.Pinmode = PINSEL_PINMODE_PULLDOWN;
    PINSEL_ConfigPin(&pinCfg);
}

// CONFIGURACION DE ADC
void configADC (void){
    ADC_Init(LPC_ADC, adcFreq);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
}

// CONFIGURACION DEL DAC
void configDAC (void){
    DAC_Init(LPC_DAC);

    DAC_CONVERTER_CFG_Type dacCfg = {0};
    dacCfg.CNT_ENA = ENABLE;
    dacCfg.DBLBUF_ENA = DISABLE;
    dacCfg.DMA_ENA = ENABLE;

    DAC_SetDMATimeOut(LPC_DAC, dacTimeout);
    DAC_configDAConverterControl(LPC_DAC, &dacCfg);
}

// CONFIGURACIÓN DEL DMA
void configDMA (void){
    GPDMA_Init();
    GPDMA_Channel_CFG_Type waveDMACfg, adcDMACfg = {0};
    GPDMA_LLI_Type wavelli, adcdaclli = {0};

    //LLI PARA MOSTRAR LA ONDA DE LA IMAGEN POR EL DAC
    wavelli.SrcAddr = (uint32_t)DAC_WAVE;
    wavelli.DstAddr = LPC_DAC -> DACCTRL;
    wavelli.NextLLI = (uint32_t)&wavelli;
    wavelli.Control = DAC_WAVE_SIZE | (1 << 18) | (3 << 15) | (1 << 26);
    // SWidth = 16 | DBSize = 16 | SI = 1

    //LLI PARA MOSTRAR LOS RESULTADOS DEL ADC POR EL DAC
    adcdaclli.SrcAddr = LPC_ADC -> ADGDR;
    adcdaclli.DstAddr = LPC_DAC -> DACCTRL;
    adcdaclli.NextLLI = (uint32_t)&adcdaclli;
    adcdaclli.Control = BUFFER_SIZE | (4 << 12) | (3 << 15);
    // SBSize = 32 | DBSize = 16

    //VOY A CONFIGURAR DOS CANALES, UNO PARA (ONDA -> DAC) Y OTRO PARA (ADC -> DAC)

    // ONDA -> DAC
    waveDMACfg.ChannelNum = 0;
    waveDMACfg.TransferSize = DAC_WAVE_SIZE;
    waveDMACfg.SrcMemAddr = (uint32_t)DAC_WAVE;
    waveDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;
    waveDMACfg.DstConn = GPDMA_CONN_DAC;
    waveDMACfg.DMALLI = (uint32_t)&wavelli;
    GPDMA_Setup(&waveDMACfg);

    // ADC -> DAC
    adcDMACfg.ChannelNum = 1;
    adcDMACfg.TransferSize = BUFFER_SIZE;
    adcDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2P;
    adcDMACfg.SrcConn = GPDMA_CONN_ADC;
    adcDMACfg.DstConn = GPDMA_CONN_DAC;
    adcDMACfg.DMALLI = (uint32_t)&adcdaclli;
    GPDMA_Setup(&adcDMACfg);

    // El programa arranca con el canal 1 habilitado (ADC -> DAC)
    GPDMA_ChannelCmd(1, ENABLE);
}

// CONFIGURACION DE LAS EXTI
void configEXTI (void){
    EXTI_Init();

    EXTI_InitTypeDef extiCfg = {0};
    extiCfg.EXTI_Line = 0;
    extiCfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    extiCfg.EXTI_polarity = EXTI_POLARITY_HIGH_ACTIVE_OR_RISING_EDGE;
    EXTI_Config(&extiCfg);

    NVIC_EnableIRQ(EINT0_IRQn);
}

//HANDLER DE EINT0
void EINT0_IRQHandler(void){
    mode = !mode;

    if(mode){
        // Switcheo los canales del DMA para que se empiece a mostrar la onda de la imagen por el DAC
        GPDMA_ChannelCmd(1, DISABLE);
        GPDMA_ChannelCmd(0, ENABLE);
    }else{
        // Switcheo los canales del DMA para que se empiece a mostrar las muestras del ADC por el DAC
        GPDMA_ChannelCmd(0, DISABLE);
        GPDMA_ChannelCmd(1, ENABLE);
    }
    EXTI_ClearEXTIFlag(EXTI_EINT0);
}

// GENERA LA FORMA DE ONDA DE LA IMAGEN
void generate_wave_form (void){
    /*
     * El DAC tiene una resolución de 10 bits (0 a 1023).
     * El rango de tensión es de 0 a VREF (usualmente 3.3V).
     * Mínimo incremento de tensión = VREF / 2^10 = 3.3V / 1024 = 3.22 mV.
     *
     * La onda tiene 382 muestras y 4 secciones.
     * 382 / 4 = 95.5. No es entero.
     * Dividimos la onda en una parte positiva (191 muestras) y una negativa (191 muestras).
     *
     * Parte positiva (191 muestras):
     * - Sección 1 (subida): 95 muestras.
     * - Pico: 1 muestra.
     * - Sección 2 (bajada): 95 muestras.
     *
     * Parte negativa (191 muestras):
     * - Sección 3 (bajada): 95 muestras.
     * - Valle: 1 muestra.
     * - Sección 4 (subida): 95 muestras.
     *
     * Amplitud máxima: 1023 (0x3FF). Amplitud media (cero): 511 (0x1FF). Amplitud mínima: 0.
     * El incremento/decremento por muestra en las rampas es: (1023 - 511) / 95 = 5.38.
     * Se usará un valor entero para el paso, por ejemplo 5, y se ajustará el valor final.
     */

    uint32_t i;
    uint16_t value;
    uint16_t mid_level = 511;
    uint16_t max_level = 1023;
    uint16_t min_level = 0;
    uint16_t step = (max_level - mid_level) / 95; // ~5

    // Sección 1: Rampa de subida (95 muestras)
    for (i = 0; i < 95; i++) {
        value = mid_level + i * step;
        DAC_WAVE[i] = (value & 0x3FF);
    }

    // Pico (1 muestra)
    DAC_WAVE[95] = (max_level & 0x3FF);

    // Sección 2: Rampa de bajada (95 muestras)
    for (i = 0; i < 95; i++) {
        value = max_level - (i + 1) * step;
        DAC_WAVE[96 + i] = (value & 0x3FF);
    }

    // Punto medio (cruzando a negativo)
    // El bucle anterior termina cerca de mid_level, el DMA continuará con la siguiente sección.

    // Sección 3: Rampa de bajada (95 muestras)
    for (i = 0; i < 95; i++) {
        value = mid_level - (i + 1) * step;
        DAC_WAVE[191 + i] = (value & 0x3FF);
    }

    // Valle (1 muestra)
    DAC_WAVE[286] = (min_level & 0x3FF);

    // Sección 4: Rampa de subida (95 muestras)
    for (i = 0; i < 95; i++) {
        value = min_level + (i + 1) * step;
        DAC_WAVE[287 + i] = (value & 0x3FF);
    }
}// EN GEMINI CONFIO