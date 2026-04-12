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
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	  MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)){}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){}
		while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)){}

    
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_4);// PD2 (X)
		MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3);// PD3 (y)
		
		// =============BUTAO JOYSTICK
		MAP_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_6);
		MAP_GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_6,
                         GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
			//============

    MAP_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);

    // CH13 = PD3 (X)
    // CH12 = PD2 (Y)
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH9);
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
        ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

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


}

/**
 * Configuração do LED RGB
 */
void ConfigLED(void)
{
		// PJ 37,38 39 -> PF1,PF2,PG0
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
    
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)){}
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOG)){}

    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3);

    MAP_GPIOPinTypeGPIOOutput(GPIO_PORTG_BASE, GPIO_PIN_0);
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
    //ConfigButton();
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
        uint8_t btn = (MAP_GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_6) == 0);

        /* ===== Envio UART ) ===== */
				UARTprintf("X:%4u Y:%4u BTN:%u\n",
									 adcValues[1], adcValues[0], btn);

        /* ===== Controle do LED RGB ===== */

				uint8_t cor1 = 0;
				uint8_t cor2 = 0;

				/* Eixo X ? vermelho e azul */
				if(adcValues[0] > 3000)
						cor1 |= GPIO_PIN_2; // vermelho

				if(adcValues[0] < 1000)
						cor2 |= GPIO_PIN_0; // azul

				/* Eixo Y ? verde */
				if(adcValues[1] > 3000)
						cor1 |= GPIO_PIN_3; // verde

				/* Botão ? apaga tudo */
				if(btn){
						cor1 = 0;
						cor2 = 0;
				}

				MAP_GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2 | GPIO_PIN_3, cor1);

        // Escreve na Porta G (Pino 0)
        MAP_GPIOPinWrite(GPIO_PORTG_BASE, GPIO_PIN_0, cor2);

        /* ===== Delay ~200 ms ===== */
        MAP_SysCtlDelay(clock / 15);
    }
}
