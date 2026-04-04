/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2023-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|		Lab 3

| __________________________________________________________________________________
*/

/**
 * @file     main.c
 * @authors   Bruno Ribeiro Basilio
							João Lucas Marques Camilo
 * @brief    Solution for lab3 to calculate the pixels of a histogram
 * @version  1.0
 * @date     01/04/2026
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/

#define PART_TM4C1294NCPDT

#include "main.h"

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
uint16_t hist[256];
uint32_t clock;
/*------------------------------------------------------------------------------
 *
 *      File scope vars
 *
 *------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/


void ConfigUART(void)
{
    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA)){}

    MAP_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    while (!MAP_SysCtlPeripheralReady(SYSCTL_PERIPH_UART0)){}

    MAP_GPIOPinConfigure(GPIO_PA0_U0RX);
    MAP_GPIOPinConfigure(GPIO_PA1_U0TX);

    MAP_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    UARTStdioConfig(0, 115200, clock);
}

void debug(void) {
    // 'static' tira o vetor da Stack para evitar Stack Overflow
    static uint16_t hist2[256] = {0}; 
    
    int tam = width1 * height1;
    

    for (int i = 0; i < tam; i++) {
        int pixel = p_start_image1[i];
        hist2[pixel] += 1;
    }
    
    for (int i = 0; i < 256; i++) {
			int diferenca = hist[i] - hist2[i]; 
        
			if (diferenca != 0)
				UARTprintf("Diferenca = %d\n", diferenca);
        
    }
    
    UARTprintf("Debug finalizado.\n");
}
	

int main(void)
{
    clock = SysCtlClockFreqSet(
        SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN |
        SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480,
        120000000);

    ConfigUART();

    UARTprintf("\nTeste com imagem do professor\n");

		// Inicia contagem de ciclos de clock (medição de desempenho)
    DWT->CTRL |= 1;

    uint32_t inicio = DWT->CYCCNT;

    // Teste imagem professor
		// Calcula o histograma da imagem (valores de 0 a 255)
		// Retorna o número total de pixels processados e os clocks 
    uint16_t total = EightBitHistogram(width1, height1, (uint8_t*)p_start_image1, hist);

    uint32_t fim = DWT->CYCCNT;

    UARTprintf("Total: %u\n", total);
    UARTprintf("Clocks: %u\n\n", fim - inicio);

		// Exibe o histograma calculado
    for (int i = 0; i < 256; i++)
    {
			UARTprintf("Valor %d -> %d\n", i, hist[i]);
    }
		debug();
		
    while(1){}
}
