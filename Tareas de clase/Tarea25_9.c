/*
 * DAC_onda triangular[]
 * DAC_onda sinusoidal[]
 * Construir un vector para ambas ondas
 * INTE0 -> +frecuencia
 * INTE1 -> -frecuencia
 * interrupción por timer0
 * fmax = 100 Hz
 * fmin = 10 Hz
 * indice de onda -> 0 - 1000
 *
 * consigna: Interrupción por MR0 y actualizar el valor del DAC
 */


#include "LPC17xx.h"

#include "lpc17xx_dac.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"



#define	OUTPUT	    (uint8_t) 1
#define PORT_ZERO	(uint8_t)	0
#define PIN_26		((uint32_t)(1<<26))

__IO uint16_t dacValue[1000];
int index = 0;

void config_timer();
void config_dac();
void config_GPIO();

int main(void) {
	for(volatile int i = 0; i< 1000; i++){
		dacValue[i] = i;
	}

	config_GPIO();
	config_timer();
	config_dac();

    while(1) {

    }
    return 0 ;
}

void config_GPIO(){
	PINSEL_CFG_Type pin_configuration;

	pin_configuration.Portnum 	=	PINSEL_PORT_0;
	pin_configuration.Pinnum	=	PINSEL_PIN_26;
	pin_configuration.Pinmode	=	PINSEL_PINMODE_TRISTATE;
	pin_configuration.Funcnum	= 	PINSEL_FUNC_2;
	pin_configuration.OpenDrain	=	PINSEL_PINMODE_NORMAL;

	PINSEL_ConfigPin(&pin_configuration);

	//GPIO_SetDir( PORT_ZERO , PIN_26 , OUTPUT );

	return;
}

void config_timer(){
	TIM_TIMERCFG_Type	struct_config;
	TIM_MATCHCFG_Type	struct_match;

	struct_config.PrescaleOption	=	TIM_PRESCALE_TICKVAL;
	struct_config.PrescaleValue		=	100;

	struct_match.MatchChannel		=	0;
	struct_match.IntOnMatch			=	ENABLE;
	struct_match.ResetOnMatch		=	ENABLE;
	struct_match.StopOnMatch		=	DISABLE;
	struct_match.ExtMatchOutputType	=	TIM_EXTMATCH_NOTHING;
	struct_match.MatchValue			=	10;


	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &struct_config);
	TIM_ConfigMatch(LPC_TIM0, &struct_match);

	TIM_Cmd(LPC_TIM0, ENABLE);

	NVIC_EnableIRQ(TIMER0_IRQn);

	return;
}

void config_dac(){
	DAC_CONVERTER_CFG_Type dacctrl_conf;

	dacctrl_conf.CNT_ENA = 0;
	dacctrl_conf.DBLBUF_ENA = 0;
	dacctrl_conf.DMA_ENA = 0;


	DAC_ConfigDAConverterControl(LPC_DAC, &dacctrl_conf);
	DAC_Init(LPC_DAC);
}

void TIMER0_IRQHandler(void){
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);
	index ++;
	if(dacValue >= 1000){
		index = 0;
		DAC_UpdateValue(LPC_DAC,dacValue[index]);
	}else{
		DAC_UpdateValue(LPC_DAC,dacValue[index]);
	}

	return;
}
