/*
Programar el microcontrolador LPC1769 para que mediante su ADC digitalice dos señales
analogicas cuyos anchos de bandas son de 10kHz cada una. Los canales utilizados deben
ser el 2 y el 4 y los datos deben ser guardados en dos regiones de memorias distintas
que permitan contar con los 20 datos de cada canal. Suponer una frecuencia de core cclk
de 100MHz. El codigo debe estar debidamente comentado.
*/

// 2 señales de 10KHz cada una
// Canales ADC 2 y 4
// Guardar 20 datos de cada canal en dos regiones de memoria distintas
// Frecuencia de core cclk de 100MHz

#include "LPC17XX.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"

#define BUFFER_SIZE 20
#define ADC_FREQ 40000 // 40KHz
#define SRAM_BANK0_ADDR 0x2007C000 // direccion de la region de memoria 1
#define SRAM_BANK1_ADDR 0x20080000 // direccion de la region de memoria 2

void configPin(void);
void configADC(void);
void configDMA(void);

int main (void){
    configPin();
    configADC();
    configDMA();

    while (1);

    return 0;
}

// ----------- CONFIGURACIONES -----------

void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};

    //ADC - Canal 2
    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 25;
    pinCfg.Funcnum = 1;
    PINSEL_ConfigPin(&pinCfg);

    //ADC - Canal 4
    pinCfg.Portnum = 1;
    pinCfg.Pinnum = 30;
    pinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&pinCfg);
}

void configADC(void){
    ADC_Init(LPC_ADC, ADC_FREQ);
    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_ChannelCmd(LPC_ADC, 2, ENABLE);
    ADC_ChannelCmd(LPC_ADC, 4, ENABLE);
}

void configDMA(void){

    GPDMA_Init();
    GPDMA_LLI_Type bank1List = {0};
    GPDMA_LLI_Type bank2List = {0};

    bank1List.SrcAddr = (uint32_t)(LPC_ADC -> ADDR2);
    bank1List.DstAddr = (uint32_t)SRAM_BANK0_ADDR;
    bank1List.NextLLI = (uint32_t)&bank1List;
    bank1List.Control = BUFFER_SIZE | (1 << 18) | (1 << 21) | (1 << 27);
    // SWidth y DWidth en 16 bits y DI = 1

    bank2List.SrcAddr = (uint32_t)(LPC_ADC -> ADDR4);
    bank2List.DstAddr = (uint32_t)SRAM_BANK1_ADDR;
    bank2List.NextLLI = (uint32_t)&bank2List;
    bank2List.Control = BUFFER_SIZE | (1 << 18) | (1 << 21) | (1 << 27);
    // SWidth y DWidth en 16 bits y DI = 1

    GPDMA_Channel_CFG_Type res1, res2 = {0};

    // Canal 0 del DMA se encarga del canal 2 del ADC
    res1.ChannelNum = 0;
    res1.TransferSize = (uint32_t)BUFFER_SIZE;
    res1.DstMemAddr = (uint32_t)SRAM_BANK0_ADDR;
    res1.TransferType = GPDMA_TRANSFERTYPE_P2M;
    res1.SrcConn = GPDMA_CONN_ADC;
    res1.DMALLI = (uint32_t)&bank1List;
    GPDMA_Setup(&res1);

    // Canal 1 del DMA se encarga del canal 2 del ADC
    res2.ChannelNum = 1;
    res2.TransferSize = (uint32_t)BUFFER_SIZE;
    res2.DstMemAddr = (uint32_t)SRAM_BANK1_ADDR;
    res2.TransferType = GPDMA_TRANSFERTYPE_P2M;
    res2.SrcConn = GPDMA_CONN_ADC;
    res2.DMALLI = (uint32_t)&bank2List;
    GPDMA_Setup(&res2);

    GPDMA_ChannelCmd(0, ENABLE);
    GPDMA_ChannelCmd(1, ENABLE);
}