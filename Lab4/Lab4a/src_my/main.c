/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2023-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|       Lab 4A
| __________________________________________________________________________________
*/

/**
 * @file     main.c
 * @authors  Bruno Ribeiro Basilio
 *           João Lucas Marques Camilo
 * @brief    Joystick reading, LED RGB control and UART communication
 * @version  1.0
 * @date     April, 2026
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/

#define PART_TM4C1294NCPDT

#include "main.h"
#include <stdio.h>

/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/

uint32_t clock;

/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/

/**
 * Configuração da UART0
 */
void ConfigUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){}

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){}

    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);

    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, clock);
}

/**
 * Configuração do ADC (Joystick)
 */
void ConfigADC(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)){}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD)){}

    // PD2 (Y) e PD3 (X)
    MAP_GPIOPinTypeADC(GPIO_PORTD_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    MAP_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    // CH13 = PD3 (X)
    // CH12 = PD2 (Y)
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH13);
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
        ADC_CTL_CH12 | ADC_CTL_IE | ADC_CTL_END);

    MAP_ADCSequenceEnable(ADC0_BASE, 0);
    MAP_ADCIntClear(ADC0_BASE, 0);
}

/**
 * Configuração do botão do joystick
 */
void ConfigButton(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)){}

    MAP_GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_1);

    MAP_GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_1,
                         GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
}

/**
 * Configuração do LED RGB
 */
void ConfigLED(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ)){}

    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTJ_BASE,
                              GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6);
}

/**
 * Função principal
 */
int main(void)
{
    clock = MAP_SysCtlClockFreqSet(
        SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
        SYSCTL_USE_PLL | SYSCTL_CFG_VCO_240,
        120000000);

    ConfigUART();
    ConfigADC();
    ConfigButton();
    ConfigLED();

    uint32_t adcValues[2];

    UARTprintf("\nLab 4A - Joystick + LED RGB\n");

    while(1)
    {
        /* ===== Leitura do ADC ===== */
        MAP_ADCProcessorTrigger(ADC0_BASE, 0);
        while(!MAP_ADCIntStatus(ADC0_BASE, 0, false)){}

        MAP_ADCSequenceDataGet(ADC0_BASE, 0, adcValues);
        MAP_ADCIntClear(ADC0_BASE, 0);

        /* ===== Leitura do botão ===== */
        uint8_t btn = (MAP_GPIOPinRead(GPIO_PORTJ_BASE, GPIO_PIN_1) == 0);

        /* ===== Envio UART ) ===== */
				UARTprintf("X:%4u Y:%4u BTN:%u\n",
									 adcValues[1], adcValues[0], btn);

        /* ===== Controle do LED RGB ===== */

				uint8_t cor = 0;

				/* Eixo X ? vermelho e azul */
				if(adcValues[0] > 3000)
						cor |= GPIO_PIN_4; // vermelho

				if(adcValues[0] < 1000)
						cor |= GPIO_PIN_6; // azul

				/* Eixo Y ? verde */
				if(adcValues[1] > 3000)
						cor |= GPIO_PIN_5; // verde

				/* Botão ? apaga tudo */
				if(btn)
						cor = 0;

				MAP_GPIOPinWrite(GPIO_PORTJ_BASE,
												 GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6,
												 cor);

        /* ===== Delay ~200 ms ===== */
        MAP_SysCtlDelay(clock / 15);
    }
}
