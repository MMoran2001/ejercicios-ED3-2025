// Consigna: Escriba un programa para generar una onda sinusoidal de baja frecuencia y demuestre el
// uso de la funcionalidad de bajo consumo del DAC.

// Lógica:
// 1. Defina un array en la memoria con al menos 32 valores discretos que representen los puntos de
// una onda sinusoidal completa.
// 2. Configure el DAC en modo de bajo consumo.
// 3. Configure el Timer 1 para generar una interrupción que se active a una frecuencia constante (la
// frecuencia de muestreo).

// Condiciones:
// o La lógica de actualización del DAC (leer el siguiente valor del array y escribirlo en el registro
// DACR) debe residir completamente en el Handler de Interrupción del Timer 1.
// o La onda sinusoidal debe tener una frecuencia final de 50 Hz.

#include "LPC17xx.h"
#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"

static uint32_t ondaSin[32]= {1, 195, 383, 556, 707, 831, 924, 981,
                    1000, 981, 924, 831, 707, 556, 383, 195,
                    1, 195, 383, 556, 707, 831, 924, 981,
                    1000, 981, 924, 831, 707, 556, 383, 195
                    };

uint32_t index = 0;

void confPin(void);
void confDac(void);
void confTimer(void);
void TIMER1_IRQHandler(void);

int main (){
    confPin();
    confDac();
    confTimer();

    while(1);
    return 0;
}

void confPin(void){
    PINSEL_CFG_Type PinCfg;

    // Configuracion para el pin del DAC
    PinCfg.Portnum = 0;
    PinCfg.Pinnum = 26;
    PinCfg.Funcnum = 2;
    PinCfg.Pinmode = PINSEL_PINMODE_PULLUP;
    PinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    PINSEL_ConfigPin(&PinCfg);
}

void confDac(void){
    DAC_Init(LPC_DAC);
    // Configuracion del DAC en modo de bajo consumo
    DAC_SetBias(LPC_DAC, DAC_MAX_CURRENT_350uA);
}

void confTimer(void){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    //Configuracion del Timer 1
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; // 1 us
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timerCfg);

    //La frecuencia de la onda deseada es de 50 Hz
    //Por lo tanto, mi frecuencia de muestreo debe ser de 50*32 = 1600 Hz
    //Entonces mi periodo de muestreo es de 1/1600 = 625 us

    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch = ENABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 625; //625 us
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
    TIM_ConfigMatch(LPC_TIM1, &matchCfg);

    TIM_Cmd(LPC_TIM1, ENABLE);
    NVIC_EnableIRQ(TIMER1_IRQn);
}

void TIMER1_IRQHandler(void){
    // Actualizacion del valor del DAC
    DAC_UpdateValue(LPC_DAC, ondaSin[index]);
    index++;
    if(index >= 32) index = 0;

    // Limpiar la bandera de interrupcion
    TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
}