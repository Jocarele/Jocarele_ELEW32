/*----------------------------------------------------------------------------
 * Name: João Lucas 
					Bruno            
	main.c
 * Purpose: LED + Botão + UARTStdio Teste
 *----------------------------------------------------------------------------
 */
#define PART_TM4C1294NCPDT

#define USER_LED1  GPIO_PIN_0
#define USER_LED2  GPIO_PIN_1
#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"


#include "BTN.h"
#include "uartstdio.h"
#include "TM4C129.h"





// =======================
// Variáveis globais
// =======================
static volatile uint32_t msTicks = 0;
static volatile uint32_t tempo_inicio = 0;
static volatile uint32_t tempo_final = 0;
static volatile uint8_t contando = 0;
uint32_t clock;

void SysTick_Handler(void);
void BTN_Callback(void);

// =======================
// SysTick
// =======================
void SysTick_Handler(void)
{
    msTicks++;
}

// =======================
// Callback do botão
// =======================
void BTN_Callback(void)
{
    UARTprintf("BOTAO!\n");

    if (contando)
    {
        tempo_final = msTicks;
        contando = 0;
				GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1|USER_LED2), 0);

        uint32_t tempo = tempo_final - tempo_inicio;

        UARTprintf("Tempo: %u ms | Clocks: %u\n",
                   tempo,
                   tempo * (SystemCoreClock / 1000));
    }
}

void ConfigureUART(void)
{
    //
    // Enable the GPIO Peripheral used by the UART.
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
		
		while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){};
    //
    // Enable UART0
    //
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	
		while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){};
    //
    // Configure GPIO Pins for UART mode.
    //
    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);
    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Initialize the UART for console I/O.
    //
    UARTStdioConfig(0, 115200, clock);
}

// =======================
// MAIN
// =======================
int main(void)
{
    // Configura clock do sistema para 120 MHz
    clock = SysCtlClockFreqSet(
        SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
        SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
        120000000);

    BTN_Init();
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }
    
    //
    // Configure the GPIO port for the LED operation.
    //
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, (USER_LED1|USER_LED2));

    // =======================
    // Configura UART0
    // =======================
		ConfigureUART();


    // Habilita interrupções globais
    IntMasterEnable();

    // Configura SysTick para 1ms
    SysTickPeriodSet(clock / 1000);
    SysTickEnable();
    SysTickIntEnable();

    // Mensagem inicial
    UARTprintf("Sistema iniciado\n");

    // Delay inicial 1s
    SysCtlDelay(clock / 3);

		
		// Turn on the LED
		GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1|USER_LED2), USER_LED1);
		
    tempo_inicio = msTicks;
    contando = 1;

    // Loop principal
    while (1)
    {
        if (contando && (msTicks - tempo_inicio >= 3000))
        {
            contando = 0;
						GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1|USER_LED2), 0);
            
            UARTprintf("Tempo limite atingido!\n");
        }
    }
}