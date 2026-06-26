#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"

#include "simula.h"

/*
 * Esta função vai estar no main.c.
 * O simulador só gera a amostra e empurra para o mesmo buffer usado pelo ADC real.
 */
extern void OscBufferPush(uint32_t valor);

#define ADC_FAKE_MIN      700
#define ADC_FAKE_MAX      3400
#define ADC_FAKE_RANGE    (ADC_FAKE_MAX - ADC_FAKE_MIN)

static uint32_t g_freq_amostragem_hz = 1000;
static uint32_t g_freq_onda_hz = 2000;
static uint32_t g_phase_q16 = 0;

void SimuladorADC_SetFrequenciaOnda(uint32_t freq_onda_hz)
{
    if (freq_onda_hz == 0)
        freq_onda_hz = 1;

    g_freq_onda_hz = freq_onda_hz;
}

static uint32_t SimuladorADC_GerarAmostraTriangular(void)
{
    uint32_t incremento;
    uint32_t fase;
    uint32_t triangular;
    uint32_t adc;

    /*
     * Fase em Q16:
     * 0 até 65535 representa um ciclo completo.
     */
    incremento = (g_freq_onda_hz * 65536UL) / g_freq_amostragem_hz;

    if (incremento == 0)
        incremento = 1;

    g_phase_q16 = (g_phase_q16 + incremento) & 0xFFFF;

    fase = g_phase_q16;

    /*
     * Gera triangular 0..65535.
     */
    if (fase < 32768UL)
    {
        triangular = fase * 2UL;
    }
    else
    {
        triangular = (65535UL - fase) * 2UL;
    }

    /*
     * Converte triangular 0..65535 para faixa ADC_FAKE_MIN..ADC_FAKE_MAX.
     */
    adc = ADC_FAKE_MIN + ((triangular * ADC_FAKE_RANGE) / 65535UL);

    if (adc > 4095)
        adc = 4095;

    return adc;
}

void SimuladorADC_Handler(void)
{
    uint32_t amostra;

    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    amostra = SimuladorADC_GerarAmostraTriangular();

    OscBufferPush(amostra);
}

void SimuladorADC_Configurar(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz)
{
    uint32_t carga;

    if (freq_amostragem_hz == 0)
        freq_amostragem_hz = 1000;

    g_freq_amostragem_hz = freq_amostragem_hz;

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}

    MAP_TimerDisable(TIMER0_BASE, TIMER_A);
    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);

    carga = (sys_clock_hz / freq_amostragem_hz) - 1;

    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, carga);
    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    TimerIntRegister(TIMER0_BASE, TIMER_A, SimuladorADC_Handler);

    MAP_IntPrioritySet(INT_TIMER0A_TM4C129, 0x80);
    MAP_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    MAP_IntEnable(INT_TIMER0A_TM4C129);

    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}

void SimuladorADC_AtualizarTaxa(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz)
{
    uint32_t carga;

    if (freq_amostragem_hz == 0)
        return;

    g_freq_amostragem_hz = freq_amostragem_hz;

    carga = (sys_clock_hz / freq_amostragem_hz) - 1;

    MAP_TimerDisable(TIMER0_BASE, TIMER_A);
    MAP_IntDisable(INT_TIMER0A_TM4C129);

    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, carga);
    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    MAP_IntEnable(INT_TIMER0A_TM4C129);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}