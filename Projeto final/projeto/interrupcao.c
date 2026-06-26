#include <stdint.h>

#include "tx_api.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"

#include "interrupcao.h"

/*
 * Objetos definidos no main.c.
 * As ISRs só usam eles.
 */
extern TX_EVENT_FLAGS_GROUP flag_0;

extern void OscBufferPush(uint32_t valor);

void Timer1A_Handler(void)
{
    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    /*
     * Acorda a thread_0 para atualizar o display.
     */
    tx_event_flags_set(&flag_0, 0x04, TX_OR);
}

void GPIOC_InterruptHandler(void)
{
    uint32_t status;

    status = MAP_GPIOIntStatus(GPIO_PORTC_BASE, true);
    MAP_GPIOIntClear(GPIO_PORTC_BASE, status);

    if (status & GPIO_PIN_6)
    {
        /*
         * Botão do joystick.
         */
        tx_event_flags_set(&flag_0, 0x01, TX_OR);
    }
}

void ADC1_InterruptHandler(void)
{
    uint32_t adc_value[1];

    MAP_ADCIntClear(ADC1_BASE, 0);
    MAP_ADCSequenceDataGet(ADC1_BASE, 0, adc_value);

    /*
     * Salva amostra no buffer do osciloscópio.
     */
    OscBufferPush(adc_value[0]);
}