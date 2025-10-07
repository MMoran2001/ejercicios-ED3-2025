/* ----CONSIGNA---- 
* Cada vez que el ADC procese un dato el dma lo mueve a la SRAM (BANCO 1) 
* direccion de destino: AHB SRAM BANCO 1
* capacidad: 256 espacios
* direccion de origen: ADC
* direccion de destino: AHB SRAM BANCO 1
* canal: 0
* modo: burst
*/

//NO ES CONSIGNA: ADC TIMER DMA DAC (Parcial)

#include <LPC17xx.h>
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpdma.h"


#define DESTINATION_ADDRESS 0x20080000 // Banco 1 de la SRAM
#define BUFFER_SIZE 256
#define ADC_FREQ 100000 // 100 kHz

uint16_t *dstAddr = (uint16_t *)DESTINATION_ADDRESS;

GPDMA_LLI_Type dmaLLI = {0};
    

void configADC (void);
void configGPIO (void);
void configDMA (void);

int main (){

    configGPIO();
    configADC();
    configDMA();

    while(1){}

    return 0;
}

void configGPIO(void){
    
    PINSEL_CFG_Type PinCfgADC = {0};

    PinCfgADC.Portnum = PINSEL_PORT_0; // Puerto 0
    PinCfgADC.Pinnum = PINSEL_PIN_23; // Pin 23 (AD0.0)
    PinCfgADC.Funcnum = PINSEL_FUNC_1; // Función 1 para ADC
    PinCfgADC.Pinmode = PINSEL_PINMODE_TRISTATE; // Modo sin pull-up ni pull-down
    PinCfgADC.OpenDrain = PINSEL_PINMODE_NORMAL; // Modo normal


    PINSEL_ConfigPin(&PinCfgADC);
}

void configADC(void){

    ADC_Init(LPC_ADC,ADC_FREQ); // Inicializa ADC con frecuencia de 100kHz
    ADC_BurstCmd(LPC_ADC,ENABLE); // Habilita modo Burst
    ADC_ChannelCmd(LPC_ADC,ADC_CHANNEL_0,ENABLE); // Habilita canal 0

}

void configDMA(void){
    
    dmaLLI.SrcAddr = (uint32_t)LPC_ADC->ADDR0;
    dmaLLI.DstAddr = (uint32_t)dstAddr;
    dmaLLI.NextLLI = (uint32_t)&dmaLLI; // buffer circular
    dmaLLI.Control = BUFFER_SIZE | 1 << 15 | 1 << 18 | 1 << 21 | 1 << 27; // 256 transfers, 16-bit width, increment destination

    GPDMA_Init();

    GPDMA_Channel_CFG_Type dmaChannelConfig = {0};
    dmaChannelConfig.ChannelNum = 0; // Canal 0
    dmaChannelConfig.TransferSize = BUFFER_SIZE; // 256 transfers
    dmaChannelConfig.SrcMemAddr = LPC_ADC->ADDR0; // Dirección fuente (Registro ADC)
    dmaChannelConfig.DstMemAddr = (uint32_t)dstAddr; // Dirección destino (Banco SRAM 1)
    dmaChannelConfig.TransferType = GPDMA_TRANSFERTYPE_P2M; // Peripheral to memory
    dmaChannelConfig.SrcConn = GPDMA_CONN_ADC; // Conexión fuente al ADC
    dmaChannelConfig.DMALLI = (uint32_t)&dmaLLI;

    GPDMA_Setup(&dmaChannelConfig);
    GPDMA_ChannelCmd(0, ENABLE); // Habilitar canal 0
}   