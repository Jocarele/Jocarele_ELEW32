// Original file in Keil\TM4C_DFP\1.1.0\Device\Include\TM4C129
// Modified for purposes of integration to TivaWare
// Changes: main setsup clock/PLL
// By: Douglas Renaux - Oct 2023

// DR changes: file inclusions
#include <stdbool.h>
#include <stdint.h>



/* This is a small demo of the high-performance ThreadX kernel.  It includes examples of eight
   threads of different priorities, using a message queue, semaphore, mutex, event flags group, 
   byte pool, and block pool.  */

#include "tx_api.h"

// DR changes: file inclusions
//#include "TM4C129.h"				//also has declaration of SystemCoreClock
extern uint32_t SystemCoreClock;
#include "driverlib/sysctl.h"		//SysCtlClockFreqSet

#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "inc/hw_memmap.h"


#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100
#define USER_LED1  GPIO_PIN_0



/* Define the ThreadX object control blocks...  */

TX_THREAD               thread_0;


TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];


/* Define the counters used in the demo application...  */

ULONG                   thread_0_counter;



/* Define thread prototypes.  */

void    thread_0_entry(ULONG thread_input);



/* Define main entry point.  */

// 2 = 1/120M * x
//x = 1*120MHZ

int main()
{
    //
    // Run from the PLL at 120 MHz.
    // Note: SYSCTL_CFG_VCO_240 is a new setting provided in TivaWare 2.2.x and
    // later to better reflect the actual VCO speed due to SYSCTL#22.
    //
    
    // see comments in sysctl.h regarding SYSCTL_CFG_VCO_480
    SystemCoreClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
																					SYSCTL_OSC_MAIN |
																					SYSCTL_USE_PLL |
																					SYSCTL_CFG_VCO_240), 120000000);
		
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);	
		while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION))
    {
    }
		GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE,USER_LED1);
    /* Enter the ThreadX kernel.  */
    tx_kernel_enter();
}


/* Define what the initial system looks like.  */

void    tx_application_define(void *first_unused_memory)
{

CHAR    *pointer = TX_NULL;


    /* Create a byte memory pool from which to allocate the thread stacks.  */
    tx_byte_pool_create(&byte_pool_0, "byte pool 0", memory_area, DEMO_BYTE_POOL_SIZE);

    /* Put system definition stuff in here, e.g. thread creates and other assorted
       create information.  */

    /* Allocate the stack for thread 0.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);

    /* Create the main thread.  */
    tx_thread_create(&thread_0, "thread 0", thread_0_entry, 0,  
            pointer, DEMO_STACK_SIZE, 
            1, 1, TX_NO_TIME_SLICE, TX_AUTO_START);


    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}



/* Define the test threads.  */

void thread_0_entry(ULONG thread_input)
{
    
    // stat j� come�a valendo os LEDs acesos
    int stat = (USER_LED1);
		stat = ~stat & 0x01; 

    while(1)
    {
        GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1), stat);
        /* Sleep for 100 ticks.  */
        tx_thread_sleep(100);
        stat = ~stat & 0x01; 
    }
}



