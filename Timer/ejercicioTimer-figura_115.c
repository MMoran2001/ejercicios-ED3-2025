/*
Escribir el código que configure el timer0 para cumplir con las
especificaciones dadas en la figura adjunta. (Pag 510 Figura 115 del
manual de usuario del LPC1769). Considerar una frecuencia de cclk de
100 Mhz y una división de reloj de periférico de 2.
*/

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"

void configTimer(void);
void TIMER0_IRQHandler(void);
void configGPIO(void);



int main (void){
    
    configTimer();
    configGPIO();

    while(1);

    return 0;
}


void configTimer(void){
    TIM_TIMERCFG_Type timerCfg;
    TIM_MATCHCFG_Type matchCfg;

    //Configuracion del timer
    timerCfg.PrescaleOption = TIM_PRESCALE_TICKVAL;
    timerCfg.PrescaleValue = 2;

    //Configuracion de match
    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch = ENABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 6;
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

    //Inicializacion del timer 0
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    //Configuracion del match
    TIM_ConfigMatch(LPC_TIM0, &matchCfg);

    //Habilitacion de la interrupcion del timer 0
    NVIC_EnableIRQ(TIMER0_IRQn);

    //Inicio del timer 0
    TIM_Cmd(LPC_TIM0, ENABLE);
}

void configGPIO(void){
    //Configuracion del pin P0.22 como salida
    PINSEL_CFG_Type pinCfg;
    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 22;
    pinCfg.Funcnum = 0;
    pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfg);
    GPIO_SetDir(0, (1<<22), 1);

    // Inicializa el pin en bajo
    GPIO_ClearValue(0, (1<<22));
}


void TIMER0_IRQHandler(void){
    //Verificacion de la interrupcion por match del canal 0
    if(TIM_GetIntStatus(LPC_TIM0, TIM_MR0_INT)){
        //Toggle del pin P0.22
        LPC_GPIO0->FIOPIN ^= (1<<22);
    }
    TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
}

