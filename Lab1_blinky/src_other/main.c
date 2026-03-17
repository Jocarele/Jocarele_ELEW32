/*----------------------------------------------------------------------------
 * Name:    Blinky.c
 * Purpose: LED Flasher
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

// Modified Mar-2025 for compatibility with compiler v6


/**
 * @file     main.c
 * @author   Douglas Renaux
 * @brief    Basic demo of kit functionality
 * @version  v2025-03 for Keil
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/
#include <stdio.h>
#include "TM4C129.h"                    // Device header
#include "LED.h"
#include "BTN.h"

/*------------------------------------------------------------------------------
 *
 *      Typedefs and constants
 *
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/
volatile uint32_t msTicks;                      /* counts 1ms timeTicks       */
//volatile const char a = __cpluspluc;          Não funcionou, acredito por ser C e não C++.
__attribute__((used)) const char date[] = __DATE__;
__attribute__((used)) const char time[] = __TIME__;
__attribute__((used)) const char file[] = __FILE__;
__attribute__((used)) int line = __LINE__;
__attribute__((used)) int stdc = __STDC__;
__attribute__((used)) int armcc_version = __ARMCC_VERSION;
__attribute__((used)) int target_arch_thumb = __thumb__;
__attribute__((used)) int arm_arch = __ARM_ARCH;
__attribute__((used)) char arch_profile = __ARM_ARCH_PROFILE;
__attribute__((used)) int stdc_version = __STDC_VERSION__;
__attribute__((used)) int gnuc_version = __GNUC__;
__attribute__((used)) int gnuc_minor = __GNUC_MINOR__;

/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
  SysTick_Handler
 *----------------------------------------------------------------------------*/
void SysTick_Handler(void) {
  msTicks++;
}

/*----------------------------------------------------------------------------
  delays number of tick Systicks (happens every 1 ms)
 *----------------------------------------------------------------------------*/
void Delay (uint32_t dlyTicks) {
  uint32_t curTicks;

  curTicks = msTicks;
  while ((msTicks - curTicks) < dlyTicks) { __NOP(); }
}


/**
 * Main function.
 *
 * @returns int    - never returns
 * @brief - simple demo presenting a text on LCD
 */


int main (void) {
	char dates = date[0];
	char times = time[0];
	char files = file[0];
  int32_t idx  = -1, dir = 1;
  uint32_t btns = 0;  


  SystemCoreClockUpdate();                      /* Get Core Clock Frequency   */

  //SER_Initialize();
  LED_Initialize();
  BTN_Initialize();

  SysTick_Config(SystemCoreClock / 1000ul);     /* Setup SysTick for 1 msec   */

  while(1) {                                    /* Loop forever               */
    btns = BTN_Get();                           /* Read button states         */

    if (btns == 0) {                            /* no push button pressed     */
      /* Calculate 'idx': 0,1,...,LED_NUM-1,LED_NUM-1,...,1,0,0,...           */
      idx += dir;
      if (idx == LED_NUM) { dir = -1; idx =  LED_NUM-1; }
      else if   (idx < 0) { dir =  1; idx =  0;         }

      LED_On (idx);                             /* Turn on LED 'idx'          */
      Delay(200);                               /* Delay 200ms                */
      LED_Off(idx);                             /* Turn off LED 'idx'         */
    }
    else {
      LED_Out ((1ul << LED_NUM) -1);
      Delay(200);                               /* Delay 200ms                */
      LED_Out (0x00);
    }
  }
}
