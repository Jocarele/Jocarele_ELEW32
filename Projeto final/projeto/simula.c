#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/rom_map.h"

#include "simula.h"

extern void OscBufferPush(uint32_t valor);
static uint32_t g_freq_amostragem_hz = 1000;
static uint32_t g_freq_onda_hz = 50;
static uint32_t g_phase_q16 = 0;

static uint32_t g_offset_adc = 2048;
static uint32_t g_amplitude_adc = 1500;

static SimuladorTipoOnda g_tipo_onda = SIM_ONDA_SENOIDAL;

/*
 * Tabela senoidal Q15 com 64 pontos.
 * Valores entre -32767 e +32767.
 */
static const int16_t seno_q15[64] = {
       0,   3212,   6393,   9512,  12539,  15446,  18205,  20787,
   23170,  25329,  27245,  28898,  30273,  31356,  32137,  32609,
   32767,  32609,  32137,  31356,  30273,  28898,  27245,  25329,
   23170,  20787,  18205,  15446,  12539,   9512,   6393,   3212,
       0,  -3212,  -6393,  -9512, -12539, -15446, -18205, -20787,
  -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
  -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
  -23170, -20787, -18205, -15446, -12539,  -9512,  -6393,  -3212
};
/**
 * @brief Limita um valor inteiro à faixa válida do ADC.
 *
 * Garante que o valor gerado pelo simulador permaneça entre 0 e 4095,
 * que corresponde à resolução de 12 bits do ADC.
 *
 * @param valor Valor inteiro a ser saturado.
 *
 * @return Valor limitado à faixa de 0 a 4095.
 */
static uint32_t SaturarADC(int32_t valor)
{
    if (valor < 0)
        return 0;

    if (valor > 4095)
        return 4095;

    return (uint32_t)valor;
}
/**
 * @brief Define a frequência da onda simulada.
 *
 * Atualiza a frequência do sinal gerado pelo simulador. Caso seja passado
 * zero, a frequência é ajustada para 1 Hz para evitar divisão inválida.
 *
 * @param freq_onda_hz Frequência desejada da onda simulada em hertz.
 *
 * @return None.
 */
void SimuladorADC_SetFrequenciaOnda(uint32_t freq_onda_hz)
{
    if (freq_onda_hz == 0)
        freq_onda_hz = 1;

    g_freq_onda_hz = freq_onda_hz;
}

/**
 * @brief Define o tipo de onda gerada pelo simulador.
 *
 * Seleciona se o simulador deve gerar uma onda senoidal ou triangular.
 *
 * @param tipo Tipo de onda definido pela enumeração SimuladorTipoOnda.
 *
 * @return None.
 */

void SimuladorADC_SetTipoOnda(SimuladorTipoOnda tipo)
{
    g_tipo_onda = tipo;
}
/**
 * @brief Configura amplitude e offset da onda simulada.
 *
 * Define a amplitude em contagens ADC e o valor central da onda.
 * A amplitude é limitada para evitar ultrapassar a faixa útil do ADC,
 * e o offset é limitado entre 0 e 4095.
 *
 * @param amplitude_adc Amplitude da onda em unidades ADC.
 * @param offset_adc Valor central da onda em unidades ADC.
 *
 * @return None.
 */
void SimuladorADC_SetAmplitudeOffset(uint32_t amplitude_adc, uint32_t offset_adc)
{
    if (amplitude_adc > 2047)
        amplitude_adc = 2047;

    if (offset_adc > 4095)
        offset_adc = 4095;

    g_amplitude_adc = amplitude_adc;
    g_offset_adc = offset_adc;
}
/**
 * @brief Reinicia a fase da onda simulada.
 *
 * Zera o acumulador de fase usado para gerar as amostras periódicas.
 *
 * @param None.
 * @return None.
 */
void SimuladorADC_ResetFase(void)
{
    g_phase_q16 = 0;
}
/**
 * @brief Atualiza o acumulador de fase do simulador.
 *
 * Calcula o incremento de fase com base na frequência da onda e na
 * taxa de amostragem. A fase é representada em formato Q16, onde
 * 0 a 65535 corresponde a um ciclo completo.
 *
 * @param None.
 * @return None.
 */
static void SimuladorADC_AtualizarFase(void)
{
    uint32_t incremento;

    /*
     * Fase Q16:
     * 0 até 65535 representa um ciclo completo.
     */
    incremento = (g_freq_onda_hz * 65536UL) / g_freq_amostragem_hz;

    if (incremento == 0)
        incremento = 1;

    g_phase_q16 = (g_phase_q16 + incremento) & 0xFFFF;
}
/**
 * @brief Gera uma amostra de onda triangular simulada.
 *
 * Atualiza a fase do sinal, calcula o valor triangular correspondente
 * e aplica amplitude e offset configurados. O resultado é limitado à
 * faixa válida do ADC.
 *
 * @param None.
 *
 * @return Amostra simulada em unidades ADC.
 */
static uint32_t SimuladorADC_GerarAmostraTriangular(void)
{
    uint32_t fase;
    uint32_t triangular;
    int32_t valor;

    SimuladorADC_AtualizarFase();

    fase = g_phase_q16;

    /*
     * triangular: 0 até 65535.
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
     * triangular fica de 0 até 65535.
     * Vamos transformar para -amplitude até +amplitude.
     */
    valor = (int32_t)triangular - 32768;
    valor = ((valor * (int32_t)g_amplitude_adc) / 32768) + (int32_t)g_offset_adc;

    return SaturarADC(valor);
}
/**
 * @brief Gera uma amostra de onda senoidal simulada.
 *
 * Atualiza a fase do sinal, consulta a tabela senoidal Q15 e aplica
 * amplitude e offset configurados. O resultado é limitado à faixa
 * válida do ADC.
 *
 * @param None.
 *
 * @return Amostra simulada em unidades ADC.
 */
static uint32_t SimuladorADC_GerarAmostraSenoidal(void)
{
    uint32_t indice;
    int32_t s;
    int32_t valor;

    SimuladorADC_AtualizarFase();

    /*
     * 64 pontos na tabela.
     * fase Q16 / 1024 = índice 0..63.
     */
    indice = g_phase_q16 >> 10;

    if (indice > 63)
        indice = 63;

    s = seno_q15[indice];

    valor = (int32_t)g_offset_adc +
            ((s * (int32_t)g_amplitude_adc) / 32767);

    return SaturarADC(valor);
}
/**
 * @brief Gera uma amostra conforme o tipo de onda selecionado.
 *
 * Chama internamente a função de geração triangular ou senoidal,
 * de acordo com a configuração atual do simulador.
 *
 * @param None.
 *
 * @return Amostra simulada em unidades ADC.
 */
static uint32_t SimuladorADC_GerarAmostra(void)
{
    if (g_tipo_onda == SIM_ONDA_TRIANGULAR)
        return SimuladorADC_GerarAmostraTriangular();

    return SimuladorADC_GerarAmostraSenoidal();
}
/**
 * @brief Rotina de interrupção do Timer0 no modo simulador.
 *
 * Limpa a interrupção do Timer0, gera uma nova amostra simulada
 * e insere essa amostra no buffer circular do osciloscópio.
 *
 * @param None.
 * @return None.
 */
void SimuladorADC_Handler(void)
{
    uint32_t amostra;

    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    amostra = SimuladorADC_GerarAmostra();

    OscBufferPush(amostra);
}
/**
 * @brief Configura o Timer0 para gerar amostras simuladas.
 *
 * Inicializa o Timer0 em modo periódico, registra a rotina de interrupção
 * do simulador e define a frequência de amostragem usada para gerar
 * as amostras falsas.
 *
 * @param sys_clock_hz Frequência do clock principal do sistema em hertz.
 * @param freq_amostragem_hz Frequência de amostragem desejada em hertz.
 *
 * @return None.
 */
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
/**
 * @brief Atualiza a taxa de amostragem do simulador.
 *
 * Reconfigura o período do Timer0 para alterar a frequência com que
 * novas amostras simuladas são geradas.
 *
 * @param sys_clock_hz Frequência do clock principal do sistema em hertz.
 * @param freq_amostragem_hz Nova frequência de amostragem em hertz.
 *
 * @return None.
 */
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