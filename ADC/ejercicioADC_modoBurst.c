/*
Configurar 4 canales del ADC para que funcionando en modo burst se
obtenga una frecuencia de muestreo en cada uno de 50Kmuestras/seg.
Suponer un CCLK = 100 MHz.
*/

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include <stdint.h>

uint32_t ADC_FREQ = 200000; //200KHz

void configGPIO(void);
void configADC(void);

int main(){

    configGPIO();
    configADC();

    while(1);

    return 0;
}

void configGPIO(void){

    PINSEL_CFG_Type ADCfg = {0};

    //configuro el canal 0 del ADC
    ADCfg.Portnum = PINSEL_PORT_0;
    ADCfg.Pinnum = PINSEL_PIN_23;
    ADCfg.Funcnum = 1;
    ADCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    ADCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&ADCfg);

    //configuro el canal 1 del ADC
    ADCfg.Pinnum = PINSEL_PIN_24;
    PINSEL_ConfigPin(&ADCfg);

    //configuro el canal 2 del ADC
    ADCfg.Pinnum = PINSEL_PIN_25;
    PINSEL_ConfigPin(&ADCfg);

    //configuro el canal 3 del ADC
    ADCfg.Pinnum = PINSEL_PIN_26;
    PINSEL_ConfigPin(&ADCfg);
}

void configADC(void){

    ADC_Init(LPC_ADC,ADC_FREQ);
    
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_1, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_2, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_3, ENABLE);

    ADC_BurstCmd(LPC_ADC, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_CONTINUOUS);
}

