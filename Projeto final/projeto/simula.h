#ifndef SIMULADOR_ADC_H
#define SIMULADOR_ADC_H

#include <stdint.h>

typedef enum {
    SIM_ONDA_TRIANGULAR = 0,
    SIM_ONDA_SENOIDAL   = 1
} SimuladorTipoOnda;

void SimuladorADC_Configurar(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz);
void SimuladorADC_AtualizarTaxa(uint32_t sys_clock_hz, uint32_t freq_amostragem_hz);
void SimuladorADC_Handler(void);

void SimuladorADC_SetFrequenciaOnda(uint32_t freq_onda_hz);
void SimuladorADC_SetTipoOnda(SimuladorTipoOnda tipo);
void SimuladorADC_SetAmplitudeOffset(uint32_t amplitude_adc, uint32_t offset_adc);
void SimuladorADC_ResetFase(void);


#endif