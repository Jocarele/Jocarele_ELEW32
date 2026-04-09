/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2023-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|       Lab 4A
| __________________________________________________________________________________
*/

/**
 * @file     main.h
 * @authors  Bruno Ribeiro Basilio
 *           João Lucas Marques Camilo
 * @brief    Header file for Lab4A - Joystick, LED RGB and UART
 * @version  1.0
 * @date     April, 2026
 ******************************************************************************/

#ifndef _MAIN_H_
#define _MAIN_H_

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

#include "inc/hw_memmap.h"

#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/adc.h"
#include "driverlib/uart.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "utils/uartstdio.h"

/*------------------------------------------------------------------------------
 *
 *      Constants
 *
 *------------------------------------------------------------------------------*/

#define LIM_SUP 3000
#define LIM_INF 1000

/*------------------------------------------------------------------------------
 *
 *      Function prototypes
 *
 *------------------------------------------------------------------------------*/

void ConfigUART(void);
void ConfigADC(void);
void ConfigButton(void);
void ConfigLED(void);

/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/

extern uint32_t clock;

#endif
