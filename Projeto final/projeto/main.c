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
 * @brief   Projeto final da disciplina de sistemas embarcados
 *            
 *            
 *
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
//#include <stdio.h>
// Bibliotecas Base da TivaWare
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

// Biblioteca ThreadX
#include "tx_api.h"

#include "grlib/grlib.h"
// Bibliotecas de Periféricos (DriverLib)
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"      
#include "driverlib/timer.h"

#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"

#include "display.h"

/*------------------------------------------------------------------------------
 *
 *      Typedefs and constants
 *
 *------------------------------------------------------------------------------*/
#define THREAD_STACK_SIZE     1024
#define DEMO_STACK_SIZE         2048
#define DEMO_BYTE_POOL_SIZE     9120
#define DEMO_BLOCK_POOL_SIZE    100
#define DEMO_QUEUE_SIZE         100
#define TELA_LARGURA 128
#define TELA_ALTURA  128
#define PART_TM4C1294NCPDT
#define TARGET_IS_TM4C129_RA1
#define TIME_SLICE_T0 TX_NO_TIME_SLICE  //TX_NO_TIME_SLICE or number
#define MUTEX 0



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
TX_MUTEX                display_mutex;
TX_BYTE_POOL            byte_pool_0;
TX_BLOCK_POOL           block_pool_0;
TX_EVENT_FLAGS_GROUP    flag_0; 
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];
int16_t width;
int16_t height;


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
void   	thread_1_entry(ULONG thread_input);
void 	ConfigADC(void);
void 	ADC1_InterruptHandler(void);
void 	GPIOC_InterruptHandler(void);
void 	SysTick_Handler(void);

/*------------------------------------------------------------------------------
 *
 *      Structs 
 *
 *------------------------------------------------------------------------------*/



#define BUFFER_SIZE 256 // Para que enquanto o adc preenche uma metade, a tela le a outra

typedef struct {
    uint32_t data[BUFFER_SIZE];
    volatile uint16_t head; 
    volatile uint16_t tail; 
    volatile uint16_t count; 
} CircularBuffer;

CircularBuffer osc_buffer = { .head = 0, .tail = 0, .count = 0 };



/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/

void SysTick_Handler(void)
{
    tx_event_flags_set(&flag_0, 0x04, TX_OR);
}




void ConfigurarADC0()
{
		
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);

    
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC)){}
	while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE)){}
	while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC0)){}
			
    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_3 | GPIO_PIN_4);

		
	// =============BUTAO JOYSTICK
	MAP_GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_6);
	MAP_GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_6,
                         GPIO_STRENGTH_2MA,
                         GPIO_PIN_TYPE_STD_WPU);
	//------------------isr
	MAP_GPIOIntTypeSet(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_FALLING_EDGE);
    GPIOIntRegister(GPIO_PORTC_BASE, GPIOC_InterruptHandler);
    MAP_GPIOIntEnable(GPIO_PORTC_BASE, GPIO_PIN_6);
	MAP_IntEnable(INT_GPIOC_TM4C129);
	//=============ADC0
			
    MAP_ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);
    // CH9 = PE3 (X)
    // CH0 = PE2 (Y)
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH9);
    MAP_ADCSequenceStepConfigure(ADC0_BASE, 0, 1,
        ADC_CTL_CH0 | ADC_CTL_IE | ADC_CTL_END);

    MAP_ADCSequenceEnable(ADC0_BASE, 0);
    MAP_ADCIntClear(ADC0_BASE, 0);
		
}

void GPIOC_InterruptHandler(void)
{
	uint32_t status = MAP_GPIOIntStatus(GPIO_PORTC_BASE, true);
	MAP_GPIOIntClear(GPIO_PORTC_BASE, status);
	
	if(status & GPIO_PIN_6)
    {
    	tx_event_flags_set(&flag_0, 0x01, TX_OR);
    }
}

void ADC1_InterruptHandler(void)
{
    uint32_t adc_value[1];

    MAP_ADCIntClear(ADC1_BASE, 0);

    MAP_ADCSequenceDataGet(ADC1_BASE, 0, adc_value);

    if (osc_buffer.count < BUFFER_SIZE) {
        osc_buffer.data[osc_buffer.head] = adc_value[0];
        osc_buffer.head = (osc_buffer.head + 1) % BUFFER_SIZE;
        osc_buffer.count++;
    } else {
        // OVERFLOW
    }
}



void ConfigurarOsciloscopioBackground(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC1);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_ADC1)) {}

    MAP_GPIOPinTypeADC(GPIO_PORTE_BASE, GPIO_PIN_2);

    // ISR
    MAP_ADCSequenceConfigure(ADC1_BASE, 0, ADC_TRIGGER_TIMER, 0);
    MAP_ADCSequenceStepConfigure(ADC1_BASE, 0, 0, ADC_CTL_CH1 | ADC_CTL_IE | ADC_CTL_END);
    
    MAP_ADCSequenceEnable(ADC1_BASE, 0);
    MAP_ADCIntClear(ADC1_BASE, 0);

    // Liga a ISR (Interrupção)
    ADCIntRegister(ADC1_BASE, 0, ADC1_InterruptHandler);
    MAP_ADCIntEnable(ADC1_BASE, 0);
    MAP_IntEnable(INT_ADC1SS0_TM4C129); 
    //TIMER0 
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    while(!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_TIMER0)) {}
    MAP_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    uint32_t carga_do_timer = (120000000 / 1000) - 1; 
    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, carga_do_timer);

    MAP_TimerControlTrigger(TIMER0_BASE, TIMER_A, true);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);
}

int main()
{
    uint32_t ui32SysClock;
    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                       SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL |
                                       SYSCTL_CFG_VCO_240), 120000000/2);

    DisplaySetup();

	

	// Configura SysTick para 300ms
    SysTickPeriodSet(clock / 300000);
    SysTickEnable();
    SysTickIntEnable();
	
    //tx_thread_sleep(100); 

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
            9, 9, TX_NO_TIME_SLICE, TX_AUTO_START);


    /* Allocate the stack for thread 1.  */
    tx_byte_allocate(&byte_pool_0, (VOID **) &pointer, DEMO_STACK_SIZE, TX_NO_WAIT);
	tx_thread_create(&thread_1, "thread 1", thread_1_entry, 1,  
            pointer, DEMO_STACK_SIZE, 
            10, 10, TX_NO_TIME_SLICE, TX_AUTO_START);
		


		
    tx_mutex_create(&display_mutex, "mutex 0", TX_NO_INHERIT);

    /* Release the block back to the pool.  */
    //tx_block_release(pointer);
	estado_atual = TELA_PRINCIPAL;
	tx_event_flags_create(&flag_0,"flag evento");
		
}



/**
 * Thread principal
 *
 * @param[in] thread_input - not used
 */
void thread_0_entry(ULONG thread_input)
{
    (void)thread_input;
    
    uint32_t amostras_tela[128]; 
    bool is_running = true;

	const char* str_vdiv = "V: 1.0V/div";
    const char* str_tdiv = "T: 1ms/div";

    ConfigurarOsciloscopioBackground();
	tx_event_flags_set(&flag_0, 0x02, TX_OR);

    while(1)
    {
		tx_event_flags_get(&flag_0, 0x04, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
        uint16_t pontos_lidos = 0;
		
			
			if (estado_atual == TELA_PRINCIPAL)
				{
					// TODO: MUTEX.Zona Crítica(buffer).
					uint32_t status = tx_interrupt_control(TX_INT_DISABLE);
					while(osc_buffer.count > 0 && pontos_lidos < 128) {
							amostras_tela[pontos_lidos] = osc_buffer.data[osc_buffer.tail];
							osc_buffer.tail = (osc_buffer.tail + 1) % BUFFER_SIZE;
							osc_buffer.count--;
							pontos_lidos++;
					}
					tx_interrupt_control(status);
					
					// Atualiza a tela
						tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
						DesenharOsciloscopio(&grContext, amostras_tela, pontos_lidos, str_vdiv, str_tdiv, is_running);
						GrFlush(&grContext);
						tx_mutex_put(&display_mutex);
				}
			else{
					uint32_t status = tx_interrupt_control(TX_INT_DISABLE);
					osc_buffer.count = 0;
					osc_buffer.head = 0;
					osc_buffer.tail = 0;
					tx_interrupt_control(status);
			}
    }
}

void thread_1_entry(ULONG thread_input)
{		
	ConfigurarADC0();
	
    (void)thread_input;
    ULONG actual_flags;
    uint32_t adc_joy[2];
	tx_event_flags_get(&flag_0, 0x03, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
    
    while(1)
    {
        // 1. MODO VISUALIZAÇÃO: 
        if (estado_atual == TELA_PRINCIPAL) {
            tx_event_flags_get(&flag_0, 0x01, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
            
            estado_atual = MENU_PRINCIPAL;
            menu_selecionado = 0;
					
			tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
            DesenharMenuPrincipal();
			tx_mutex_put(&display_mutex);
            tx_thread_sleep(50); 
        }

        // 2. MODO CONFIGURAÇÃO:
        while (estado_atual != TELA_PRINCIPAL) 
        {
            // Lê o X e o Y do Joystick via ADC0
            MAP_ADCProcessorTrigger(ADC0_BASE, 0);
            while(!MAP_ADCIntStatus(ADC0_BASE, 0, false)) {}
            MAP_ADCIntClear(ADC0_BASE, 0);
            MAP_ADCSequenceDataGet(ADC0_BASE, 0, adc_joy);
            
            uint32_t joy_x = adc_joy[0]; 
            uint32_t joy_y = adc_joy[1]; 
            
            // Verifica se clicou de novo (TX_NO_WAIT para não travar o joystick)
            bool clicou = (tx_event_flags_get(&flag_0, 0x01, TX_OR_CLEAR, &actual_flags, TX_NO_WAIT) == TX_SUCCESS);
            // NAVEGAÇÃO: MENU PRINCIPAL
            if (estado_atual == MENU_PRINCIPAL) {
                // Navega para Cima / Baixo (Limites de 0 a 4)
								
                if (joy_y > 3000 && menu_selecionado > 0) { 
                    menu_selecionado--; 
					
					tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                    DesenharMenuPrincipal(); 
					tx_mutex_put(&display_mutex);
									
                }
                if (joy_y < 1000 && menu_selecionado < 4) { 
                    menu_selecionado++; 
					tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                    DesenharMenuPrincipal(); 
					tx_mutex_put(&display_mutex);
                }
				if (joy_x < 200) {
					estado_atual = TELA_PRINCIPAL;
					tx_thread_sleep(50);
				}
                
                if (clicou) {
                    if (menu_selecionado == 0) estado_atual = MENU_TAXA;
                    if (menu_selecionado == 1) estado_atual = MENU_VDIV;
                    if (menu_selecionado == 2) estado_atual = MENU_HDIV;
                    if (menu_selecionado == 3) estado_atual = MENU_NIVEL_TRIG;
                    if (menu_selecionado == 4) estado_atual = MENU_BORDA_TRIG;
                }
            }
            // NAVEGAÇÃO: SUBMENUS 
            else {
                int* ptr_indice = NULL;
                int max_indice = 2; 
                const char* titulo = "";
                int valor = 0;
				const char* unidade = "";
								 
                if (estado_atual == MENU_TAXA) {
                    ptr_indice = &indice_taxa; titulo = "Taxa Amostragem"; valor = valores_taxa[*ptr_indice];unidade = "V/div";
                } else if (estado_atual == MENU_VDIV) {
                    ptr_indice = &indice_vdiv; titulo = "Escala Vertical"; valor = valores_vdiv[*ptr_indice];unidade = "V/div";
                } else if (estado_atual == MENU_HDIV) {
                    ptr_indice = &indice_hdiv; titulo = "Escala de Tempo"; valor = valores_hdiv[*ptr_indice];unidade = "ms/div";
                } else if (estado_atual == MENU_NIVEL_TRIG) {
                    ptr_indice = &indice_nivel_trig; titulo = "Nivel Trigger"; valor = valores_nivel[*ptr_indice];unidade = "V";
                } else if (estado_atual == MENU_BORDA_TRIG) {
                    ptr_indice = &indice_borda_trig; titulo = "Borda Trigger"; valor = (*ptr_indice);unidade = str_borda_trig[*ptr_indice];
                    max_indice = 1;
                }
								
				tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                DesenharSubMenu(titulo, valor,unidade,true);
				tx_mutex_put(&display_mutex);
								
                // Navega para Esquerda / Direita (Altera o valor)
                if (joy_x < 1000 && *ptr_indice > 0) { (*ptr_indice)--; }
                if (joy_x > 3000 && *ptr_indice < max_indice) { (*ptr_indice)++; }
				
				/*
                if (joy_x < 200) {
                    estado_atual = MENU_PRINCIPAL;
					tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                    DesenharMenuPrincipal();
					tx_mutex_put(&display_mutex);
                    tx_thread_sleep(30); 
                }
				*/

                // TODO: Chamar as funções de hardware. 
                if (clicou) {
                    
	                if (estado_atual == MENU_TAXA) {
	                    ptr_indice = &indice_taxa; valor = valores_taxa[*ptr_indice];
	                } else if (estado_atual == MENU_VDIV) {
	                    ptr_indice = &indice_vdiv;  valor = valores_vdiv[*ptr_indice];
	                } else if (estado_atual == MENU_HDIV) {
	                    ptr_indice = &indice_hdiv; valor = valores_hdiv[*ptr_indice];
	                } else if (estado_atual == MENU_NIVEL_TRIG) {
	                    ptr_indice = &indice_nivel_trig;  valor = valores_nivel[*ptr_indice];
	                } else if (estado_atual == MENU_BORDA_TRIG) {
	                    ptr_indice = &indice_borda_trig; valor = (*ptr_indice);unidade = str_borda_trig[*ptr_indice];
	                }
					estado_atual = TELA_PRINCIPAL; 
                }
            }

            // Pausa de 15 ticks
            tx_thread_sleep(15); 
        }
    }
}


