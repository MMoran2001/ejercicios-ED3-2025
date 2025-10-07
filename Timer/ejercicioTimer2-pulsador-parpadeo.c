/*
Escribir un programa para que por cada presión de un pulsador, la
frecuencia de parpadeo de un led disminuya a la mitad debido a la
modificación del prescaler del Timer 2. El pulsador debe producir una
interrupción por EINT1 con flanco descendente.
*/

#include "LPC17xx.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_exti.h"

#define LED (1 << 22)
#define BOTON (1<<11)
#define MAX_PRESCALER_VALUE 128000

static uint32_t PrescalerValue = 1000;
static uint32_t matchValue = 1000;

void configGPIO(void);
void configTimer(uint32_t prescalerValue);
void configEXTI(void);
void TIMER2_IRQHandler(void);
void EINT1_IRQHandler(void);

int main (void){
    
    configGPIO();
    configTimer(PrescalerValue);
    configEXTI();

    GPIO_ClearValue(0, LED);

    while(1);

    return 0;
}

void configGPIO(void){
    PINSEL_CFG_Type pinCfgBoton = {0};
    PINSEL_CFG_Type pinCfgLed = {0};
    
    //Configuracion del pin 2.11 como EINT1
    pinCfgBoton.Portnum = 2;
    pinCfgBoton.Pinnum = 11;
    pinCfgBoton.Funcnum = 1; //Funcion 1 para EINT1
    pinCfgBoton.Pinmode = PINSEL_PINMODE_PULLUP;
    pinCfgBoton.OpenDrain = PINSEL_PINMODE_NORMAL;

    pinCfgLed.Portnum = 0;
    pinCfgLed.Pinnum = 22;
    pinCfgLed.Funcnum = 0; //Funcion 0 para GPIO
    pinCfgLed.Pinmode = PINSEL_PINMODE_TRISTATE;
    pinCfgLed.OpenDrain = PINSEL_PINMODE_NORMAL;

    PINSEL_ConfigPin(&pinCfgBoton);
    PINSEL_ConfigPin(&pinCfgLed);
    GPIO_SetDir(0, LED, 1); //P0.22 como salida
    GPIO_SetDir(2, BOTON, 0); //P2.11 como entrada
}

void configTimer(uint32_t prescalerValue){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    //Configuracion de timer
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = prescalerValue; //1 ms

    //Configuracion de match
    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch = ENABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = matchValue; //1 segundo
    matchCfg.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;

    //Inicializacion del timer 2
    TIM_Init(LPC_TIM2, TIM_TIMER_MODE, &timerCfg);

    //Configuracion del match
    TIM_ConfigMatch(LPC_TIM2, &matchCfg);

    //Habilitacion de la interrupcion del timer 2
    NVIC_EnableIRQ(TIMER2_IRQn);

    //Prioridad 2 a la interrupcion del timer 2
    NVIC_SetPriority(TIMER2_IRQn, 2);

    //Inicio del timer 2
    TIM_Cmd(LPC_TIM2, ENABLE);
}

void configEXTI(void){

    EXTI_InitTypeDef extiCfg = {0};

    extiCfg.EXTI_Line = EXTI_EINT1;
    extiCfg.EXTI_Mode = EXTI_MODE_EDGE_SENSITIVE;
    extiCfg.EXTI_polarity = EXTI_POLARITY_LOW_ACTIVE_OR_FALLING_EDGE;

    EXTI_Init();
    EXTI_Config(&extiCfg);

    //Prioridad 1 a la interrupcion por EINT1
    NVIC_SetPriority(EINT1_IRQn, 1);

    NVIC_EnableIRQ(EINT1_IRQn);
}

void TIMER2_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM2, TIM_MR0_INT)){
        LPC_GPIO0 -> FIOPIN ^= LED;
    }
    TIM_ClearIntPending(LPC_TIM2, TIM_MR0_INT);
}

void EINT1_IRQHandler(void){
    //Disminuir la frecuencia de parpadeo a la mitad
    PrescalerValue = PrescalerValue * 2;

    if(PrescalerValue > MAX_PRESCALER_VALUE){
        PrescalerValue = 1000;
    }

    TIM_Cmd(LPC_TIM2, DISABLE);
    configTimer(PrescalerValue);

    EXTI_ClearEXTIFlag(EXTI_EINT1);
}