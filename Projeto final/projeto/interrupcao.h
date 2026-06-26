#ifndef INTERRUPCAO_H
#define INTERRUPCAO_H

#include <stdint.h>

void Timer1A_Handler(void);
void GPIOC_InterruptHandler(void);
void ADC1_InterruptHandler(void);

#endif