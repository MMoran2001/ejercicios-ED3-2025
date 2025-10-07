/*
Frecuencia de core cclk 80 MHz
Interrupcion externa EINT0 (P2.10) por flanco de subida
Señal analogica de 80KHz 

Consigna: Programar el microcontrolador LPC1769 en un código en lenguaje C, con una frecuencia de core cclk de 80MHz, para que
mediante una interrupción externa en el PIN 2.10 por flanco ascendente, se comande secuencialmente las siguientes 
funcionalidades:

Mediante el ADC se debe digitalizar una señal analógica cuyo ancho de banda es de 80 KHz, y su amplitud de pico
máxima positiva es de 3,3V.

- Los datos deben ser guardados, cada 1 segundo, utilizando punteros (ADC_POINTER)
- Los datos deben ser guardados, cada 1 segundo, utilizando DMA (ADC_DMA)

En ambos casos, desde la primera mitad de la memoria SRAM ubicada en el bloque ahb SRAM-bank 0 de manera tal que permita
almacenar todos los datos posibles que esta memoria permita, mediante un buffer circular conservando siempre las ultimas
muestras.

*Este parrafo no lo hice*
Mediante el hardware GPDMA se deben transferir los datos desde la primera mitad de la memoria SRAM ubicada en el
bloque ahb sram-bank 0 hacia la segunda mitad de dicha memoria, en donde se deben promediar todos los datos, y dicho
valor digitalizado promediado debe ser transmitido por la UART0 en dos tramas de 1 byte que representan la parte entera 
y decimal del mismo.
*/

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_exti.h"

#define SRAM_BANK0_ADDR 0x2007C000
#define SRAM_BANK0_HALF_SIZE 0x2000 // 8KB = 8192 bytes
#define SRAM_BANK0_MID_ADDR (SRAM_BANK0_ADDR + SRAM_BANK0_HALF_SIZE)
#define BUFFER_SIZE (SRAM_BANK0_HALF_SIZE / sizeof(uint16_t)) // 4096 muestras de 16 bits

static uint32_t adcFreq = 160000;
static uint8_t mode = 0; // 0: ADC_POINTER, 1: ADC_DMA

static uint16_t* adc_buffer = (uint16_t*) SRAM_BANK0_ADDR; // Puntero al buffer en SRAM_BANK0

void configPin(void);
void configADC(void);
void configDMA(void);
void configTimer(void);
void configEXTI(void);
void EINT0_IRQHandler(void);
void ADC_IRQHandler(void);

int main (void){
    configPin();
    configADC();
    configDMA();
    configTimer();
    configEXTI();


    // El micro va a empezar en modo ADC_POINTER transformando y guardando datos en un buffer circular
    while(1);

    return 0;
}

/*------------------ CONFIGURACIONES ------------------*/

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};

    //Configuracion del pin 2.10 como EINT0
    pinCfg.Portnum = 2;
    pinCfg.Pinnum = 10;
    pinCfg.Funcnum = 1;
    pinCfg.Pinmode = PINSEL_PINMODE_PULLDOWN;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pinCfg);

    //Configuracion del pin 0.23 como AD0.0
    pinCfg.Portnum = 0; 
    pinCfg.Pinnum = 23;
    pinCfg.Funcnum = 1;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pinCfg);
}

// CONFIGURACION DEL ADC
void configADC(void){
    ADC_Init(LPC_ADC, adcFreq);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE); // Habilito la interrupcion por canal 0
    NVIC_EnableIRQ(ADC_IRQn);
}

// CONFIGURACION DEL TIMER
void configTimer(void){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    //Configuracion del Timer 0
    timerCfg.PrescaleOption = TIM_PRESCALE_TICKVAL;
    timerCfg.PrescaleValue = 1; // 1 tick
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    //Configuracion de match
    matchCfg.MatchChannel = 1; // Match en MR1
    matchCfg.IntOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 125; //  (80MHz/4) / 160KHz = 125
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;

    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
}

// CONFIGURACION DEL DMA
void configDMA(void){
    GPDMA_Channel_CFG_Type GPDMACfg = {0};
    GPDMA_LLI_Type lliBank0 = {0};
    GPDMA_LLI_Type lliBank1 = {0};

    GPDMA_Init();
    // LLI para el primer banco
    lliBank0.SrcAddr = LPC_ADC -> ADDR0; //Direccion del registro de datos del ADC
    lliBank0.DstAddr = SRAM_BANK0_ADDR; //Direccion de la primera mitad de la SRAM
    lliBank0.NextLLI = (uint32_t) &lliBank1; //Direccion de la siguiente estructura LLI
    lliBank0.Control = BUFFER_SIZE | (1 << 12) | (1 << 13) | (1 << 15) | (1 << 16) | (1 << 27);
    // Tamaño de la tranferencia (1024), tamaño de fuente y destino (16 bits), incremento de destino.

    // LLI para el segundo banco
    lliBank1.SrcAddr = LPC_ADC -> ADDR0; //Direccion del registro de datos del ADC
    lliBank1.DstAddr = SRAM_BANK0_MID_ADDR; //Direccion de la segunda mitad
    lliBank1.NextLLI = (uint32_t) &lliBank0; //Direccion de la estructura del banco 0 para hacer el buffer circular
    lliBank1.Control = BUFFER_SIZE | (1 << 12) | (1 << 13) | (1 << 15) | (1 << 16) | (1 << 27);
    // IDEM primer banco

    // Configuracion del canal DMA
    GPDMACfg.ChannelNum = 0; //Canal 0
    GPDMACfg.TransferSize = BUFFER_SIZE; //Tamaño de la transferencia
    GPDMACfg.SrcMemAddr = 0; //No se usa en este caso
    GPDMACfg.DstMemAddr = SRAM_BANK0_ADDR;
    GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_P2M; //Periferico a memoria
    GPDMACfg.SrcConn = GPDMA_CONN_ADC; //Conexion al ADC
    GPDMACfg.DstConn = 0; //No se usa en este caso
    GPDMACfg.DMALLI = (uint32_t) &lliBank0; //Direccion de la primera estructura LLI

    GPDMA_Setup(&GPDMACfg);
    GPDMA_ChannelCmd(0, DISABLE); //Inicialmente deshabilitado, cuando cambie de modo se habilita
}

// CONFIGURACION DE EXTI
void configEXTI(void){
    EXTI_Init();

    EXTI_InitTypeDef extiCfg = {0};

    extiCfg.EXTI_Line = EXTI_EINT0; // INTERUPCION EXTERNA 0
    extiCfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE; // SENSIBILIDAD POR FLANCO
    extiCfg.EXTI_polarity = EXTI_POLARITY_HIGH_ACTIVE_OR_RISING_EDGE; // FLANCO DE SUBIDA
    EXTI_Config(&extiCfg);

    NVIC_EnableIRQ(EINT0_IRQn);
}

/*------------------ HANDLERS ------------------*/

// HANDLER DE LA INTERRUPCION EXTERNA EINT0
void EINT0_IRQHandler(void){
    if(mode == 0){
        mode = 1;
        GPDMA_ChannelCmd(0, ENABLE); // Habilito el canal DMA
        NVIC_DisableIRQ(ADC_IRQn); // Deshabilito la interrupcion por ADC
    }else{
        mode = 0;
        GPDMA_ChannelCmd(0, DISABLE); // Deshabilito el canal DMA
        NVIC_EnableIRQ(ADC_IRQn); // Habilito la interrupcion por ADC
    }

    EXTI_ClearEXTIFlag(EXTI_EINT0); // Limpio la bandera de la interrupcion externa
}

void ADC_IRQHandler(void){
    
    if(mode == 0){ // estoy en modo ADC_POINTER??
    static uint32_t index = 0; // Indice para el buffer circular en modo punteros

    if(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE)){ // Si la conversion esta lista y limpia la bandera de interrupcion
        uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0); // Leo el valor del ADC (12 bits)
        
        adc_buffer[index] = value; // Guardo el valor en el buffer circular
        index++;

        if(index >= BUFFER_SIZE){
            index = 0;
        }
   }
}else{return;}
}