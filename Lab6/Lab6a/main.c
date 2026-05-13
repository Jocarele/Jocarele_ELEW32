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

#include "driverlib/sysctl.h"

/*------------------------------------------------------------------------------
 *
 *      Typedefs and constants
 *
 *------------------------------------------------------------------------------*/
#define THREAD_STACK_SIZE     1024

/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/
TX_THREAD thread_0;

/*------------------------------------------------------------------------------
 *
 *      File scope vars
 *
 *------------------------------------------------------------------------------*/
static UCHAR thread_0_stack[THREAD_STACK_SIZE];

/*------------------------------------------------------------------------------
 *
 *      Function prototypes
 *
 *------------------------------------------------------------------------------*/
void tx_application_define(void * first_unused_memory);

void thread_0_entry(ULONG thread_input);

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
int main(int argc, char ** argv)
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
void tx_application_define(void * first_unused_memory)
{
    /*
     * Cria a thread principal
     */
    tx_thread_create(
            &thread_0,                 /* Control block             */
            "thread 0",                /* Thread name               */
            thread_0_entry,            /* Entry function            */
            0,                         /* Entry input               */
            thread_0_stack,            /* Stack start               */
            THREAD_STACK_SIZE,         /* Stack size                */
            1,                         /* Priority                  */
            1,                         /* Preemption threshold      */
            TX_NO_TIME_SLICE,          /* Time slice                */
            TX_AUTO_START              /* Auto start                */
    );
}


/**
 * Thread principal
 *
 * @param[in] thread_input - not used
 */
void thread_0_entry(ULONG thread_input)
{
    while(1)
    {
        /*
         * Suspende a thread por 100 ticks
         */
        tx_thread_sleep(100);
    }
}