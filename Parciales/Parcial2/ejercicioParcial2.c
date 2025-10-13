/*
Programar el microcontrolador LPC1769 en un codigo de lenguaje C para que,
utilizando un timer y un pin de capture de esta placa sea posible demodular
una señal PWM que ingresa por dicho pin (calcular el ciclo de trabajo y el periodo)
y sacar una tension continua proporcional al ciclo de trabajo a traves del DAC de rango
dinamico 0-2V con un rate de actualizacion de 0.5s del promedio de los ultimos valores
obtenidos en la captura
*/

#include "LPC17xx.h"
#include "lpc17xx_pinsel.h"
#include "lpc17xx_timer.h"
#include "lpc17xx_dac.h"

void configPin(void);
void configTimer(void);
void configDAC(void);
void TIMER0_IRQHandler(void);
void TIMER1_IRQHandler(void);

volatile uint32_t rising_edge_time = 0;
volatile uint32_t falling_edge_time = 0;
volatile uint32_t period = 0;
volatile uint32_t pulse_width = 0;
volatile uint8_t edge_is_rising = 1; // 1 para subida, 0 para bajada

#define MAX_SAMPLES 50
volatile uint32_t duty_cycle_samples[MAX_SAMPLES] = {0};
volatile uint8_t sample_count = 0;

int main (void){
    configPin();
    configTimer();
    configDAC();

    while(1);

    return 0;
}

// CONFIGURACION DE PINES
void configPin(void){
    PINSEL_CFG_Type pinCfg = {0};
    
    //CAPTURE
    pinCfg.Portnum = 1;
    pinCfg.Pinnum = 26;
    pinCfg.Funcnum = 3;
    pinCfg.OpenDrain = PINSEL_PINMODE_NORMAL;
    pinCfg.Pinmode = PINSEL_PINMODE_TRISTATE;
    PINSEL_ConfigPin(&pinCfg);

    //DAC
    pinCfg.Portnum = 0;
    pinCfg.Pinnum = 26;
    pinCfg.Funcnum = 2;
    PINSEL_ConfigPin(&pinCfg);

    //MATCH 1.0
    pinCfg.Portnum = 1;
    pinCfg.Pinnum = 22;
    pinCfg.Funcnum = 3;
    PINSEL_ConfigPin(&pinCfg);
}

// CONFIGURACION DE TIMER
void configTimer(void){
    TIM_TIMERCFG_Type timerCfg = {0};
    TIM_CAPTURECFG_Type captureCfg = {0};
    TIM_MATCHCFG_Type matchCfg = {0};

    //TIMER0
    timerCfg.PrescaleOption = TIM_PRESCALE_USVAL;
    timerCfg.PrescaleValue = 1; // 1us
    TIM_Init(LPC_TIM0, TIM_TIMER_MODE, &timerCfg);

    captureCfg.CaptureChannel = 0;
    captureCfg.RisingEdge = ENABLE;
    captureCfg.FallingEdge = DISABLE;
    captureCfg.IntOnCaption = ENABLE;
    TIM_ConfigCapture(LPC_TIM0, &captureCfg);

    NVIC_EnableIRQ(TIMER0_IRQn);

    TIM_Cmd(LPC_TIM0, ENABLE);

    //TIMER1
    TIM_Init(LPC_TIM1, TIM_TIMER_MODE, &timerCfg);
    matchCfg.MatchChannel = 0;
    matchCfg.IntOnMatch = ENABLE;
    matchCfg.StopOnMatch = DISABLE;
    matchCfg.ResetOnMatch = ENABLE;
    matchCfg.MatchValue = 500000 - 1; // 0.5s
    TIM_ConfigMatch(LPC_TIM1, &matchCfg);

    NVIC_EnableIRQ(TIMER1_IRQn);

    TIM_Cmd(LPC_TIM1, ENABLE);
}

// CONFIGURACION DE DAC
void configDAC(void){
    DAC_CONVERTER_CFG_Type cfgDAC = {0};
    DAC_Init(LPC_DAC);

    cfgDAC.CNT_ENA = DISABLE;
    cfgDAC.DBLBUF_ENA = DISABLE;
    cfgDAC.DMA_ENA = DISABLE;

    DAC_ConfigDAConverterControl(LPC_DAC, &cfgDAC);
    DAC_UpdateValue(LPC_DAC, 0); // Inicializo el DAC en 0
}

//HANDLER DE TIMER0
void TIMER0_IRQHandler(void){
    if(TIM_GetIntCaptureStatus(LPC_TIM0, TIM_CR0_INT)){
        if(edge_is_rising){
            //Capture un flanco de subida
            uint32_t current_time = TIM_GetCaptureValue(LPC_TIM0, 0);
            period = current_time - rising_edge_time;
            rising_edge_time = current_time;

            //Calcular el ciclo de trabajo si hay datos validos
            if (period > 0 && sample_count < MAX_SAMPLES) {
                pulse_width = falling_edge_time - rising_edge_time; // Esto puede dar negativo si el TC se reseteó
                if (falling_edge_time < rising_edge_time) { // Manejo de overflow del contador
                    pulse_width = (0xFFFFFFFF - rising_edge_time) + falling_edge_time;
                }
                // Guardar el ciclo de trabajo (0-100)
                duty_cycle_samples[sample_count] = (pulse_width * 100) / period;
                sample_count++;
            }

            // Configuro para cambiar el proximo flanco (de bajada)
            LPC_TIM0 -> CCR &= ~(1 << 0);
            LPC_TIM0 -> CCR |= (1 << 1);
            edge_is_rising = 0;
        }
        
        else{
            // Se capturo un flanco de bajada
            falling_edge_time = LPC_TIM0->CR0;

            //Ahora configuro para capturar el proximo flanco (de subida)
            LPC_TIM0 -> CCR &= ~(1 << 1);
            LPC_TIM0 -> CCR |= (1 << 0);
            edge_is_rising = 1;
        }
    }

    TIM_ClearIntCapturePending(LPC_TIM0, TIM_CR0_INT);
}

//HANDLER DE TIMER1
void TIMER1_IRQHandler(void){
    if(TIM_GetIntStatus(LPC_TIM1, TIM_MR0_INT)){
        if (sample_count > 0) {
            uint32_t sum = 0;
            for (int i = 0; i < sample_count; i++) {
                sum += duty_cycle_samples[i];
            }
            uint32_t avg_duty = sum / sample_count;

            // Escalar el ciclo de trabajo (0-100) al valor del DAC (0-1023)
            // La consigna pide 0-2V. Si VREF es 3.3V, el valor máximo para 2V es (2/3.3)*1023 = 620
            uint32_t dac_value = (avg_duty * 620) / 100;
            
            DAC_UpdateValue(LPC_DAC, dac_value);

            // Resetear para el próximo promedio
            sample_count = 0;
        }
        TIM_ClearIntPending(LPC_TIM1, TIM_MR0_INT);
    }
}