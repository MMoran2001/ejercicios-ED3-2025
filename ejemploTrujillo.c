#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpdma.h"

void configGPIO(void);
void configDAC(void);
void configDMA(void);

#define ADDRESS ((0x2007C000)) // Direccion del registro DACR
#define DAC_COUNT (100) // Valor de timeout para el contador DMA

uint16_t *srcAddr = (uint16_t *)ADDRESS;

int main(void) {
    configGPIO();
    configDAC();
    configDMA();

    while (1) {
        // Bucle principal vacío, la operación se maneja por DMA
    }
    return 0;
}

void configGPIO(void) {
    PINSEL_CFG_Type PinCfg;

    PinCfg.Funcnum = PINSEL_FUNC_2; // Función 2 para DAC
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PinCfg.Portnum = PINSEL_PORT_0; // Puerto 0
    PinCfg.Pinnum = PINSEL_PIN_26; // Pin 26 (DAC output)

    PINSEL_ConfigPin(&PinCfg);
}

void configDAC(void) {
    DAC_Init(LPC_DAC);

    DAC_CONVERTER_CFG_Type dacConfig;
    dacConfig.CNT_ENA = ENABLE;
    dacConfig.DBLBUF_ENA = ENABLE;
    dacConfig.DMA_ENA = ENABLE;

    DAC_ConfigDAConverterControl(LPC_DAC, &dacConfig);
    DAC_SetDMATimeOut(LPC_DAC, DAC_COUNT);
}

void configDMA(void) {
    GPDMA_LLI_Type dmaLLI;
    dmaLLI.SrcAddr = (uint32_t)srcAddr;
    dmaLLI.DstAddr = (uint32_t)&LPC_DAC->DACR;
    dmaLLI.NextLLI = (uint32_t)&dmaLLI; // Apunta a sí mismo para transferencias circulares
    dmaLLI.Control = 256 | 1 << 12 | 1 << 18 | 1 << 21; // 256 transfers, 16-bit width, increment source, no increment destination
    
    GPDMA_Init();

    GPDMA_Channel_CFG_Type dmaChannelConfig;

    dmaChannelConfig.ChannelNum = 0; // Canal 0
    dmaChannelConfig.SrcMemAddr = (uint32_t)srcAddr; // Dirección fuente
    dmaChannelConfig.DstMemAddr = (uint32_t)&LPC_DAC->DACR; // Dirección destino (registro DACR)
    dmaChannelConfig.TransferSize = 256; // Transferir 256 palabras (16 bits cada una)
    dmaChannelConfig.TransferType = GPDMA_TRANSFERTYPE_M2P; // Memoria a periférico
    dmaChannelConfig.DstConn = GPDMA_CONN_DAC; // Conexión destino al DAC
    dmaChannelConfig.DMALLI = (uint32_t)&dmaLLI; 

    GPDMA_Setup(&dmaChannelConfig);
    GPDMA_ChannelCmd(0, ENABLE); // Habilitar canal 0
}