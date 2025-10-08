/*
INPUT: ADC Canal 2: Modo burst
       Señal de entrada de hasta 30KHz
       Cada 16 muestras se promedia

OUTPUT: DAC con el promedio de las muestras
*/

#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"
#include "lpc17xx_dac.h"

#define BUFFER_SIZE 16 // Cantidad de muestras a promediar
#define ADC_FREQ 60000 // 60KHz

static uint16_t adc_buffer[BUFFER_SIZE] = {0}; // Buffer circular para las muestras del ADC
static uint8_t buffer_index = 0; // Indice del buffer circular
static uint16_t promedio = 0;


void configPin(void);
void configADC(void);
void configDMA(void);
void ADC_IRQHandler(void);

int main (void){
    configPin();
    configADC();
    configDMA();

    DAC_Init(LPC_DAC); // inicio el DAC

    while(1);
    return 0;
}

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type PinCfg = {0};
    
    //ADC
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 25;
    PinCfg.Funcnum = 1;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&PinCfg);

    //DAC
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Funcnum = 2;
    PINSEL_ConfigPin(&PinCfg);
}

// CONFIGURACION DE ADC
void configADC (void){
    ADC_Init(LPC_ADC, ADC_FREQ);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
}

// CONFIGURACION DE DMA
void configDMA(void){
    GPDMA_Init();
    GPDMA_Channel_CFG_Type channelConfig = {0};
    GPDMA_LLI_Type lliConfig = {0};

    lliConfig.SrcAddr = (uint32_t)&LPC_ADC->ADDR2;
    lliConfig.DstAddr = (uint32_t)adc_buffer;
    lliConfig.Control = BUFFER_SIZE | (3 << 12) | (3 << 15) | (1 << 27) | (1 << 31);// Transfer size de 16
                                                                                    // SBsize y DBSize de 16 bits
                                                                                    // DI = 1
                                                                                    // I = 1
    lliConfig.NextLLI = &lliConfig;
    
    channelConfig.ChannelNum = 0;
    channelConfig.SrcMemAddr = 0;
    channelConfig.DstMemAddr = lliConfig.DstAddr;
    channelConfig.TransferSize = BUFFER_SIZE;
    channelConfig.SrcConn = GPDMA_CONN_ADC;
    channelConfig.DstConn = 0;
    channelConfig.TransferType = GPDMA_TRANSFERTYPE_P2M;
    channelConfig.DMALLI = &lliConfig;

    GPDMA_Setup(&channelConfig);
    GPDMA_ChannelCmd(0,ENABLE);
    NVIC_EnableIRQ(DMA_IRQn);
}

// HANDLER DMA

void DMA_IRQHandler(void){
    if(GPDMA_IntGetStatus(GPDMA_STAT_INT, 0)){
        //TODO: promedio y mandar al DAC
    }
}



