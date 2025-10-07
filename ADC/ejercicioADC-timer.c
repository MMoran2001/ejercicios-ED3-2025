/*
Adquirir dos señales analogica de 16kHz por el pin P0.23 y P0.24 cada 1s.
Utilizar la funcionalidad de MATCH del pin P1.29 como base de
tiempo para la adquisición.
*/

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_adc.h"
#include <stdio.h>

#define PIN_AD00        (1<<23) //P0.23
#define PIN_AD01        (1<<24) //P0.24
#define PIN_MATCH       (1<<29) //P1.29

void configPin(void);
void configADC(void);
void configTimer(void);
void ADC_IRQHandler(void);


int main (void){
    configPin();
    configADC();
    configTimer();

    while(1);
    return 0;
}

void configPin(void){
    PINSEL_CFG_Type PinCfg = {0};

    /*
     * Init ADC pin connect
     * AD0.0 on P0.23
     * AD0.1 on P0.24
     */

    PinCfg.Funcnum = 1;
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinCfg.Pinnum = 23;
    PinCfg.Portnum = 0;
    PINSEL_ConfigPin(&PinCfg);

    PinCfg.Pinnum = 24;
    PINSEL_ConfigPin(&PinCfg);

    GPIO_SetDir(0, PIN_AD00, 0); // P0.23 como entrada
    GPIO_SetDir(0, PIN_AD01, 0); // P0.24 como entrada

    PinCfg.Funcnum = 3;
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PinCfg.Pinnum = 29;
    PinCfg.Portnum = 1;
    PINSEL_ConfigPin(&PinCfg);

    GPIO_SetDir(1, PIN_MATCH, 1); // P1.29 como salida
}

void configADC(void){

    ADC_Init(LPC_ADC, 64000); //ADC conversion rate = 64KHz
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_1, ENABLE);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01); //Inicia conversion con Match0.1
    ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_FALLING); //Bajando el pulso de Match0.1
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0|ADC_ADINTEN1, ENABLE); //Habilito interrupcion de los canales 0 y 1

    NVIC_EnableIRQ(ADC_IRQn);
}

void configTimer(void){
    TIM_TIMERCFG_Type timerCfg ={0};
    TIM_MATCHCFG_Type matchCfg ={0};

    //Configuracion del timer
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; //1us

    //Configuracion de match
    matchCfg.MatchChannel = 1;
    matchCfg.IntOnMatch = DISABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 1000000; //1s
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE; //Genera pulso en el pin

    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void ADC_IRQHandler(void){

    uint16_t adc_value0 = 0;
    uint16_t adc_value1 = 0;
    if(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE) == SET){
        adc_value0 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0);
    }

    if(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_1, ADC_DATA_DONE) == SET){
        adc_value1 = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_1);
    }

    printf("ADC0: %d - ADC1: %d \n", adc_value0, adc_value1); //Cada 1s me muestra el valor de ambas señales

}
