#include <stdint.h>
#include <stdbool.h>

#include "tx_api.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/timer.h"
#include "driverlib/rom_map.h"

#include "hardware.h"

extern TX_EVENT_FLAGS_GROUP flag_0;
extern uint32_t ui32SysClock;

extern void OscBufferPush(uint32_t valor);

/**
 * @brief Rotina de interrupção do Timer1A.
 *
 * Limpa a interrupção do Timer1A e sinaliza a thread principal para
 * atualizar a tela do osciloscópio.
 *
 * @param None.
 * @return None.
 */
void Timer1A_Handler(void)
{
    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    tx_event_flags_set(&flag_0, 0x04, TX_OR);
}

/**
 * @brief Configura o Timer1A para gerar interrupções periódicas.
 *
 * O Timer1A é usado como base de tempo para atualização da tela.
 * A cada interrupção, uma flag do ThreadX é acionada para permitir
 * que a thread de visualização redesenhe o display.
 *
 * @param periodo_ms Período de interrupção em milissegundos.
 *
 * @return None.
 */
void ConfigurarTimer1A(uint32_t periodo_ms)
{
    uint32_t carga;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER1)) {}

    MAP_TimerDisable(TIMER1_BASE, TIMER_A);
    MAP_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);

    carga = ((ui32SysClock / 1000) * periodo_ms) - 1;

    MAP_TimerLoadSet(TIMER1_BASE, TIMER_A, carga);
    MAP_TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    TimerIntRegister(TIMER1_BASE, TIMER_A, Timer1A_Handler);

    MAP_IntPrioritySet(INT_TIMER1A_TM4C129, 0x80);
    MAP_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    MAP_IntEnable(INT_TIMER1A_TM4C129);
    MAP_TimerEnable(TIMER1_BASE, TIMER_A);
}

/**
 * @brief Rotina de interrupção do botão do joystick.
 *
 * Verifica a interrupção no pino PC6, limpa a flag correspondente
 * e sinaliza uma flag do ThreadX indicando que houve clique no botão.
 *
 * @param None.
 * @return None.
 */
void GPIOC_InterruptHandler(void)
{
    uint32_t status;

    status = MAP_GPIOIntStatus(GPIO_PORTC_BASE, true);
    MAP_GPIOIntClear(GPIO_PORTC_BASE, status);

    if (status & GPIO_PIN_6)
    {
        tx_event_flags_set(&flag_0, 0x01, TX_OR);
    }
}

/**
 * @brief Configura o ADC0 e o botão do joystick.
 *
 * Configura os canais analógicos usados para leitura dos eixos do joystick
 * e configura o pino PC6 como entrada digital com pull-up e interrupção
 * por borda de descida.
 *
 * @param None.
 * @return None.
 */
void ConfigurarADC0(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)) {}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)) {}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)) {}

   
     //Joystick analógico.
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4);


     //butão Botão do joystick: PC6 com pull-up.

    MAP_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_6);
    MAP_GPIOPadConfigSet(GPIO_PORTC_BASE,
                         GPIO_PIN_6,
                         GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);

    MAP_GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTC_BASE, GPIOC_InterruptHandler);
    MAP_GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_6);
    MAP_IntEnable(INT_GPIOC_TM4C129);
    MAP_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH9);
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
                                 ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

    MAP_ADCSequenceEnable(ADC0_BASE, 0);
    MAP_ADCIntClear(ADC0_BASE, 0);
}

/**
 * @brief Rotina de interrupção do ADC1.
 *
 * Lê uma amostra do ADC1 sequenciador 0 e a insere no buffer circular
 * usado pela lógica do osciloscópio.
 *
 * @param None.
 * @return None.
 */
void ADC1_InterruptHandler(void)
{
    uint32_t adc_value[1];

    MAP_ADCIntClear(ADC1_BASE, 0);
    MAP_ADCSequenceDataGet(ADC1_BASE, 0, adc_value);

    OscBufferPush(adc_value[0]);
}
/**
 * @brief Configura a aquisição real do osciloscópio via ADC1.
 *
 * Configura o pino PD2/AIN13 como entrada analógica, habilita o ADC1
 * acionado por Timer0 e registra a interrupção responsável por armazenar
 * as amostras no buffer circular.
 *
 * @param None.
 * @return None.
 */
void ConfigurarOsciloscopioBackground(void)
{
    uint32_t carga_do_timer;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)) {}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)) {}

    //Entrada PD2
    MAP_GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2);

    //TIMER ADC1
    MAP_ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_TIMER, 0);

    MAP_ADCSequenceStepConfigure(ADC1_BASE,0,0,ADC_CTL_CH13 | ADC_CTL_IE | ADC_CTL_END);

    MAP_ADCSequenceEnable(ADC1_BASE, 0);
    MAP_ADCIntClear(ADC1_BASE, 0);

    ADCIntRegister(ADC1_BASE, 0, ADC1_InterruptHandler);
    MAP_ADCIntEnable(ADC1_BASE, 0);
    MAP_IntEnable(INT_ADC1SS0_TM4C129);

		//TAXA DE AMOSTRAGEM INICIAL
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}

    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    carga_do_timer = (ui32SysClock / 1000) - 1;

    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, carga_do_timer);
    MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}