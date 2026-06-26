#ifndef SIMULADOR_ADC_H
#define SIMULADOR_ADC_H

#include <stdint.h>

void SimuladorADC_Configurar(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz);
void SimuladorADC_AtualizarTaxa(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz);
void SimuladorADC_Handler(void);

void SimuladorADC_SetFrequenciaOnda(uint32_t freq_onda_hz);

#endif