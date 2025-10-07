/*
Usar match 1.0 para disparar la conversión del ADC cada 1 seg
*/

#ifdef __USE_CMSIS
#include "LPC17xx.h"
#endif

#include "stdio.h"
/*!
 * Header files to project. Include library
 */

#include "lpc17xx_timer.h"
#include "lpc17xx_gpio.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_adc.h"

// TODO: insert other definitions and declarations here
__IO uint32_t adc_value;

#define	OUTPUT	    (uint8_t) 1
#define INPUT	    (uint8_t) 0

#define PIN_22		((uint32_t)(1<<22))
#define PIN_23		((uint32_t)(1<<23))
#define PORT_ZERO	(uint8_t)	0
#define PORT_ONE	(uint8_t)	1

#define _ADC_CHANNEL	ADC_CHANNEL_0
#define _ADC_INT		ADC_ADINTEN0

void config_GPIO(void);
void config_timer(void);
void config_ADC(void);
void ADC_IRQHandler(void);

void TIMER0_IRQHandler(void){
	TIM_ClearIntPending(LPC_TIM0, TIM_MR0_INT);

	if (GPIO_ReadValue(PORT_ZERO)&PIN_22) {
		GPIO_ClearValue(PORT_ZERO, PIN_22);
	} else {
		GPIO_SetValue(PORT_ZERO,PIN_22);
	}

	return;
}

int main(void) {

	config_GPIO();
	config_timer();
	GPIO_SetValue(PORT_ZERO,PIN_22);

	while(1) {}
    return 0 ;
}

void config_GPIO(){
	PINSEL_CFG_Type pin_configuration;

	pin_configuration.Portnum 	=	PINSEL_PORT_1;
	pin_configuration.Pinnum	=	PINSEL_PIN_22;
	pin_configuration.Pinmode	=	PINSEL_PINMODE_PULLUP;
	pin_configuration.Funcnum	= 	PINSEL_FUNC_3;
	pin_configuration.OpenDrain	=	PINSEL_PINMODE_NORMAL;

	PINSEL_ConfigPin(&pin_configuration);

	GPIO_SetDir( PORT_ONE , PIN_22 , OUTPUT );

	return;
}

void config_timer(){
	TIM_TIMERCFG_Type	struct_config;
	TIM_MATCHCFG_Type	struct_match;

	struct_config.PrescaleOption	=	TIM_PRESCALE_USVAL;
	struct_config.PrescaleValue		=	1;

	struct_match.MatchChannel		=	0;
	struct_match.IntOnMatch			=	ENABLE;
	struct_match.ResetOnMatch		=	ENABLE;
	struct_match.StopOnMatch		=	DISABLE;
	struct_match.ExtMatchOutputType	=	TIM_EXTMATCH_NOTHING;
	struct_match.MatchValue			=	1000000;

	TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &struct_config);
	TIM_ConfigMatch(LPC_TIM0, &struct_match);

	TIM_Cmd(LPC_TIM0, ENABLE);

	NVIC_EnableIRQ(TIMER0_IRQn);

	return;
}

void config_ADC(void){

	ADC_Init(LPC_ADC,100000);
	ADC_IntConfig(LPC_ADC,_ADC_INT,ENABLE);
	ADC_ChannelCmd(LPC_ADC, _ADC_CHANNEL, ENABLE);

	NVIC_SetPriority(ADC_IRQn, (9));
	NVIC_EnableIRQ(ADC_IRQn);

	return;
}

void ADC_IRQHandler(void)
{
	if (ADC_ChannelGetStatus(LPC_ADC,_ADC_CHANNEL,ADC_DATA_DONE))
		{
			adc_value =  ADC_ChannelGetData(LPC_ADC,_ADC_CHANNEL);
			printf("valor ADC: %d",adc_value);
		}

	adc_value = 0;


}
