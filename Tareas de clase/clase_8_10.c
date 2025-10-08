/*
INPUT:
- Sensor lineal: 0-100°C
- Trigger: MAT0.1 (cada 100ms)

OUTPUT:
- LED:
	- VERDE: 0-40 grados
	- AMARILLO: 41-60 grados
	- ROJO: 61-100 grados & 10 muestras consecutivas en el rango
*/


#include "LPC17xx.h"
#include "lpc17xx_adc.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"

#define OUTPUT 1
#define PORT_ZERO 0
#define PORT_THREE 3
#define LED_ROJO (1 << 22) // P0.22
#define LED_VERDE (1 << 25) //P3.25
#define adc_freq 100000 // 100kHz
#define PRESCALER_VALUE 1000 // 1ms
#define MATCH_VALUE 50 - 1 // 50ms
#define UMBRAL_VERDE 1638 // (40 * 4095) / 100 (40 grados)
#define UMBRAL_AMARILLO 2457 // (60 * 4095) / 100 (60 grados)
#define MAX_SAMPLES 10

void configPin(void);
void configADC(void);
void configTimer(void);

void main (void){
    configPin();
    configADC();
    configTimer();

    while(1);
    return 0;
}

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};

    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 22;
    pinCfg.Funcnum = 0;
    pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&pinCfg); // Led rojo

    pinCfg.Portnum = 3;
    pinCfg.Pinnum = 25; // Led verde
    PINSEL_ConfigPin(&pinCfg);

    pinCfg.Portnum = 1;
    pinCfg.Pinnum = 29;
    pinCfg.Funcnum = 3; // MAT0.1
    PINSEL_ConfigPin(&pinCfg);

    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 23;
    pinCfg.Funcnum = 1; // AD0.0
    PINSEL_ConfigPin(&pinCfg);

    GPIO_SetDir(PORT_ZERO, LED_ROJO, OUTPUT); // Salida
    GPIO_SetDir(PORT_THREE, LED_VERDE, OUTPUT); // Salida

    GPIO_ClearValue(PORT_ZERO, LED_ROJO);
    GPIO_ClearValue(PORT_THREE, LED_VERDE); //inicio los leds apagados
}

// CONFIGURACION DEL ADC
void configADC(void){
    ADC_Init(LPC_ADC, adc_freq);
    ADC_ChannelCmd(LPC_ADC, ADC_CHANNEL_0, ENABLE);
    ADC_BurstCmd(LPC_ADC, DISABLE);
    ADC_StartCmd(LPC_ADC, ADC_START_ON_MAT01);
    ADC_IntConfig(LPC_ADC, ADC_ADINTEN0, ENABLE); // Habilito la interrupcion por canal 0
    ADC_EdgeStartConfig(LPC_ADC, ADC_START_ON_RISING);
    NVIC_EnableIRQ(ADC_IRQn);
}

// CONFIGURACION DEL TIMER
void configTimer(void){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = PRESCALER_VALUE; // 1000us = 1ms
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    matchCfg.MatchChannel = 1; // MAT0.1
    matchCfg.IntOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = MATCH_VALUE; // 50ms
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    TIM_Cmd(LPC_TIM0, ENABLE); // Inicio el timer
}

// HANDLER DE LA INTERRUPCION DEL ADC
void ADC_IRQHandler(void){
    if(ADC_ChannelGetStatus(LPC_ADC, ADC_CHANNEL_0, ADC_DATA_DONE)){
        static uint8_t contador = 0;
        uint16_t value = ADC_ChannelGetData(LPC_ADC, ADC_CHANNEL_0); // Leo el valor del ADC (12 bits)
        GPIO_ClearValue(PORT_ZERO, LED_ROJO);
        GPIO_ClearValue(PORT_THREE, LED_VERDE);

        if(value <= UMBRAL_VERDE){
            GPIO_SetValue(PORT_THREE, LED_VERDE);
            GPIO_ClearValue(PORT_ZERO, LED_ROJO);
            contador = 0;
        }else if(value > UMBRAL_VERDE && value <= UMBRAL_AMARILLO){
            GPIO_SetValue(PORT_THREE, LED_VERDE);
            GPIO_SetValue(PORT_ZERO, LED_ROJO);
            contador = 0;
        }else if(value > UMBRAL_AMARILLO){
            contador++;
            if(contador >= MAX_SAMPLES){
                GPIO_ClearValue(PORT_THREE, LED_VERDE);
                GPIO_SetValue(PORT_ZERO, LED_ROJO);
                contador = 0;
            }
            GPIO_SetValue(PORT_ZERO, LED_ROJO);
            GPIO_SetValue(PORT_THREE, LED_VERDE);
        }
    }
}