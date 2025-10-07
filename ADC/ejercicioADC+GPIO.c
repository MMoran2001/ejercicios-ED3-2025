/*
En base al ejemplo 1, modificar el valor del Match para que la
frecuencia de muestreo del ADC sea de 20 Kmuestras/seg. El resultado
de la conversión debe mostrarse por 12 pines de salida del puerto
GPIO0.
*/

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_gpio.h"
#include <stdint.h>


void configGPIO(void);
void configADC(void);
void configTimer(void);

int main(void){
    configGPIO();
    configADC();
    configTimer();

    while(1);
    return 0;
}

void configGPIO(void){
    PINSEL_CFG_Type pinCfg = {0};

    //configuro el canal 0 del ADC
    pinCfg.Portnum = PINSEL_PORT_0;
    pinCfg.Pinnum = PINSEL_PIN_23;
    pinCfg.Funcnum = 1;
    pinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pinCfg);

    //Supongo que los pines 0.0 a 0.11 son GPIO por defecto
    //configuro los pines del GPIO0 como salida
    GPIO_SetDir(0, 0xFFF, 1);
}

void configADC(void){
    ADC_Init(LPC_ADC, 20000);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);

    NVIC_EnableIRQ(ADC_IRQn);
}

void configTimer(void){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; // 1us
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    matchCfg.MatchChannel = 1;
    matchCfg.IntOnMatch = DISABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 50;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    TIM_Cmd(LPC_TIM0, ENABLE);
}