#include "main.h"


// Endereço final: 0x4006.4008
#define PORTN_PIN1_DATA (*((volatile uint32_t *)0x40064008))

// contadorees de ciclo
volatile uint32_t ciclos_TivaWare = 0;
volatile uint32_t ciclos_C_Direto = 0;
volatile uint32_t ciclos_Assembly_inline = 0;
volatile uint32_t ciclos_Assembly = 0;


volatile uint32_t clock = 0;

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

int main(void)
{
    uint32_t start_time, end_time;
    uint32_t i;

    // 1. Configura o clock do sistema para a frequência máxima (120 MHz)
    clock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | 
													SYSCTL_OSC_MAIN | 
                           SYSCTL_USE_PLL | 
                           SYSCTL_CFG_VCO_480), 120000000);
		ConfigUART();

    // 2. Habilita o periférico Port N e configura o pino 1 como saída
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
    while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPION));
    GPIOPinTypeGPIOOutput(GPIO_PORTN_BASE, GPIO_PIN_1);

    SysTickPeriodSet(0x00FFFFFF); // 16.777.215 ciclos
    SysTickEnable();

    // TÉCNICA 1: TivaWare (DriverLib)
	
    start_time = SysTickValueGet(); // Inicia a contagem
    
    for(i = 0; i < 1000; i++)
    {
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, GPIO_PIN_1); // HIGH
        GPIOPinWrite(GPIO_PORTN_BASE, GPIO_PIN_1, 0);          // LOW
    }
    
    end_time = SysTickValueGet(); // Para a contagem
    ciclos_TivaWare = start_time - end_time; // Calcula o delta (conta pra baixo)

    // TÉCNICA 2: C 
		
    start_time = SysTickValueGet();
    
    for(i = 0; i < 1000; i++)
    {
        PORTN_PIN1_DATA = 0x02; // HIGH (Seta o bit 1)
        PORTN_PIN1_DATA = 0x00; // LOW  (Zera o bit 1)
    }
    
    end_time = SysTickValueGet();
    ciclos_C_Direto = start_time - end_time;

    // TÉCNICA 3: Assembly Inline
    start_time = SysTickValueGet();
    
    __asm (
        " mov r0, #1000 \n"           
        " ldr r1, =0x40064008 \n"     
        " mov r2, #2 \n"              
        " mov r3, #0 \n"              
        "loop_pulso: \n"
        " str r2, [r1] \n"           
        " str r3, [r1] \n"           
        " subs r0, r0, #1 \n"         
        " bne loop_pulso \n"         
    );
    
    end_time = SysTickValueGet();
    ciclos_Assembly_inline = start_time - end_time;
		
		// TÉCNICA 4: Assembly outline

		start_time = SysTickValueGet();
		thousand_pulse();
		end_time = SysTickValueGet();

		ciclos_Assembly = start_time - end_time;
		
		UARTprintf("ciclos_TivaWare = %d\n", ciclos_TivaWare);
		UARTprintf("ciclos_C_Direto = %d\n", ciclos_C_Direto);
		UARTprintf("ciclos_Assembly_inline = %d\n", ciclos_Assembly_inline);
		UARTprintf("ciclos_Assembly = %d\n", ciclos_Assembly);
		

	while(1){}
    
}