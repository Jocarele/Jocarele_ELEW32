/*----------------------------------------------------------------------------
 * Name:    BTN.c
 * Purpose: low Level Push Button functions
 * Note(s):
 *----------------------------------------------------------------------------
 * This file is part of the uVision/ARM development tools.
 * This software may only be used under the terms of a valid, current,
 * end user licence from KEIL for a compatible version of KEIL software
 * development tools. Nothing else gives you the right to use this software.
 *
 * This software is supplied "AS IS" without warranties of any kind.
 *
 * Copyright (c) 2014 Keil - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#define PART_TM4C1294NCPDT

#include <stdint.h>
#include <stdbool.h>
#include "BTN.h"
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "BTN.h"
#include "uartstdio.h"



void GPIOJ_Handler(void);
void BTN_Init(void);
extern void BTN_Callback(void);

void GPIOJ_Handler(void)
{
    uint32_t status = GPIOIntStatus(GPIO_PORTJ_BASE, true);
    GPIOIntClear(GPIO_PORTJ_BASE, status);

    UARTprintf("BOTAO PRESS\n"); // teste
    BTN_Callback();               // agora chama a função do main.c
}

void BTN_Init(void)
{
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);

    while (!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOJ));

    GPIOPinTypeGPIOInput(GPIO_PORTJ_BASE, GPIO_PIN_0);

    GPIOPadConfigSet(GPIO_PORTJ_BASE, GPIO_PIN_0,
                     GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

    GPIOIntTypeSet(GPIO_PORTJ_BASE, GPIO_PIN_0, GPIO_FALLING_EDGE);

    GPIOIntEnable(GPIO_PORTJ_BASE, GPIO_PIN_0);

    IntEnable(INT_GPIOJ);
}