/*----------------------------------------------------------------------------
 * Name:    main.c
 * Purpose: LED + Botão + UARTStdio Teste
 *----------------------------------------------------------------------------
 */

#define PART_TM4C1294NCPDT

#include <stdint.h>
#include <stdbool.h>
#include "TM4C129.h"
#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

#include "LED.h"
#include "BTN.h"
#include "uartstdio.h"

// =======================
// Variáveis globais
// =======================
static volatile uint32_t msTicks = 0;
static volatile uint32_t tempo_inicio = 0;
static volatile uint32_t tempo_final = 0;
static volatile uint8_t contando = 0;

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
        LED_Off(0);

        uint32_t tempo = tempo_final - tempo_inicio;

        UARTprintf("Tempo: %u ms | Clocks: %u\n",
                   tempo,
                   tempo * (SystemCoreClock / 1000));
    }
}

// =======================
// MAIN
// =======================
int main(void)
{
    // Configura clock do sistema para 120 MHz
    uint32_t clock = SysCtlClockFreqSet(
        SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
        SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
        120000000);

    // Inicializa LED e Botão
    LED_Initialize();
    BTN_Init();

    // =======================
    // Configura UART0
    // =======================
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));

    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_UART0));

    UARTStdioConfig(0, 115200, clock);

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

    // Acende LED e inicia contagem
    LED_On(0);
    tempo_inicio = msTicks;
    contando = 1;

    // Loop principal
    while (1)
    {
        if (contando && (msTicks - tempo_inicio >= 3000))
        {
            contando = 0;
            LED_Off(0);
            UARTprintf("Tempo limite atingido!\n");
        }
    }
}