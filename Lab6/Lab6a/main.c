/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2026-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|       Lab6a
| __________________________________________________________________________________
*/

/**
 * @file     main.c
 * @author   Bruno Ribeiro Basilio
						 João Lucas Marques Camilo
 * @brief    Exemplo básico utilizando o RTOS ThreadX na TM4C1294.
 *            O sistema configura o clock em 120 MHz, inicializa
 *            o kernel ThreadX e cria uma thread simples.
 *
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

#include "tx_api.h"

#include "TM4C129.h"

#include "inc/hw_memmap.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

#include "driverlib/rom.h"
#include "driverlib/rom_map.h"


#include "driverlib/sysctl.h"

/*------------------------------------------------------------------------------
 *
 *      Typedefs and constants
 *
 *------------------------------------------------------------------------------*/
#define THREAD_STACK_SIZE     1024
#define DEMO_STACK_SIZE         1024
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100
#define USER_LED1  GPIO_PIN_1
#define USER_LED2  GPIO_PIN_0
#define USER_LED3  GPIO_PIN_4
#define LOOPS_PARA_100MS  307212


/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/
TX_THREAD               thread_0;
TX_THREAD               thread_1;
TX_THREAD               thread_2;
TX_QUEUE                queue_0;
TX_SEMAPHORE            semaphore_0;
TX_MUTEX                mutex_0;
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];

/*------------------------------------------------------------------------------
 *
 *      File scope vars
 *
 *------------------------------------------------------------------------------*/
//static UCHAR thread_0_stack[THREAD_STACK_SIZE];
//volatile uint32_t *DWT_CTRL = (uint32_t *)0xE0001000;
//volatile uint32_t *DWT_CYCCNT = (uint32_t *)0xE0001004;
//volatile uint32_t *SCB_DEMCR = (uint32_t *)0xE000EDFC;

/*------------------------------------------------------------------------------
 *
 *      Function prototypes
 *
 *------------------------------------------------------------------------------*/
void tx_application_define(void * first_unused_memory);

void    thread_0_entry(ULONG thread_input);
void    thread_1_entry(ULONG thread_input);
void    thread_2_entry(ULONG thread_input);

/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/
/**
 * Main function.
 *
 * @param[in] argc - not used
 * @param[in] argv - not used
 *
 * @returns int - not used
 */
 void blink_led(uint8_t led, uint32_t repetitions)
{
		for (uint32_t i=0;i<= repetitions;i++)
	{
		if (led == USER_LED3)
		{
				GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1 | USER_LED2 | USER_LED3), 0);
				GPIOPinWrite(GPIO_PORTF_BASE, (USER_LED1 | USER_LED2 | USER_LED3), led);			
		}
		else
		{
				GPIOPinWrite(GPIO_PORTF_BASE, USER_LED3, 0);
				GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1 | USER_LED2 | USER_LED3), led);

		}
		GPIOPinWrite(GPIO_PORTN_BASE, (USER_LED1 | USER_LED2 | USER_LED3), 0);
		GPIOPinWrite(GPIO_PORTF_BASE, USER_LED3, 0);
	}
	
}

int main()
{
    /*
     * Configura o clock da TM4C1294 para 120 MHz
     */
    SystemCoreClock = SysCtlClockFreqSet(
                            SYSCTL_XTAL_25MHZ |
                            SYSCTL_OSC_MAIN   |
                            SYSCTL_USE_PLL    |
                            SYSCTL_CFG_VCO_240,
                            120000000);
	
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
		while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION)) {}
		GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, (USER_LED1 | USER_LED2));

		// Ativa o Portal F (para o LED D3)
		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
		while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF)) {}
		GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, USER_LED3);
			
		



    /*
     * Inicializa o kernel ThreadX
     */
    tx_kernel_enter();

    return 0;
}


/**
 * Define os objetos iniciais do sistema
 *
 * @param[in] first_unused_memory - memória não utilizada
 */
void    tx_application_define(void *first_unused_memory)
{
		(void)first_unused_memory;
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
            10, 0, TX_NO_TIME_SLICE, TX_AUTO_START);


    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
		tx_thread_create(&thread_1, "thread 1", thread_1_entry, 1,  
            pointer, DEMO_STACK_SIZE, 
            11, 0, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the stack for thread 2.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
		tx_thread_create(&thread_2, "thread 2", thread_2_entry, 2,  
            pointer, DEMO_STACK_SIZE, 
            12, 0, TX_NO_TIME_SLICE, TX_AUTO_START);

    /* Allocate the message queue.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_QUEUE_SIZE*sizeof(ULONG), TX_NO_WAIT);
		
    /* Create the mutex used by thread 6 and 7 without priority inheritance.  */
    tx_mutex_create(&mutex_0, "mutex 0", TX_INHERIT);

    /* Allocate the memory for a small block pool.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_BLOCK_POOL_SIZE, TX_NO_WAIT);

    /* Create a block memory pool to allocate a message buffer from.  */
    tx_block_pool_create(&block_pool_0, "block pool 0", sizeof(ULONG), pointer, DEMO_BLOCK_POOL_SIZE);

    /* Allocate a block and release the block memory.  */
    tx_block_allocate(&block_pool_0, (VOID **) &pointer, TX_NO_WAIT);

    /* Release the block back to the pool.  */
    tx_block_release(pointer);
}





/**
 * Thread principal
 *
 * @param[in] thread_input - not used
 */
void thread_0_entry(ULONG thread_input)
{
		(void)thread_input;
		uint8_t led;
		led = USER_LED1;
	/*
    *SCB_DEMCR |= 0x01000000;
    *DWT_CYCCNT = 0;
    *DWT_CTRL |= 1;
    uint32_t ciclos_inicio = *DWT_CYCCNT;
    blink_led(USER_LED1, 1); 
    uint32_t ciclos_fim = *DWT_CYCCNT;
		volatile uint32_t ciclos_gastos = ciclos_fim - ciclos_inicio;	

	*/
    while(1)
		{
				tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);
				blink_led(led, (LOOPS_PARA_100MS * 3) / 2);
				tx_mutex_put(&mutex_0);
				blink_led(led, (LOOPS_PARA_100MS * 3) / 2);
				blink_led(led,LOOPS_PARA_100MS*3);
        tx_thread_sleep(70);

    }
}

void    thread_1_entry(ULONG thread_input)
{
		(void)thread_input;
		uint8_t led;
		led = USER_LED2;

    /* This thread simply sits in while-forever-sleep loop.  */
    while(1)
    {

			/* Sleep for 10 ticks.  */
      
			blink_led(led,LOOPS_PARA_100MS*5);
			tx_thread_sleep(100);
    }
}
void    thread_2_entry(ULONG thread_input)
{
		(void)thread_input;
		uint8_t led;
		led = USER_LED3;

    /* This thread simply sits in while-forever-sleep loop.  */
    while(1)
    {

        /* Sleep for 10 ticks.  */			
				tx_mutex_get(&mutex_0, TX_WAIT_FOREVER);
				blink_led(led, (LOOPS_PARA_100MS * 8) / 2);
				tx_mutex_put(&mutex_0);
				blink_led(led, (LOOPS_PARA_100MS * 8) / 2);
				tx_thread_sleep(320);

		}
}


