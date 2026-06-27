#ifndef HARDWARE_H
#define HARDWARE_H

#include <stdint.h>

void ConfigurarTimer1A(uint32_t periodo_ms);
void ConfigurarADC0(void);
void ConfigurarOsciloscopioBackground(void);

void Timer1A_Handler(void);
void GPIOC_InterruptHandler(void);
void ADC1_InterruptHandler(void);

#endif