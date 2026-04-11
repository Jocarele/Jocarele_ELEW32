# Relatório do Lab4b

**Nomes:**
* João Lucas Marques Camilo
* Bruno Ribeiro Basilio

---

## 1. Introdução

Este relatório descreve a comparação em tempo em relação àS funções em C, C+TIVAWARE e assembly.

## 2. Planejamento do processo de desenvolvimento

* Revisão instruções assembly.
* Revisão Lab2, sobre inicialização terminal TTY.
* Pesquisar inicialização LED 1.
* Pesquisar em como gerar um pulso na maior frequencia;
* Testes, depuração e sincronização com o repositório GitHub.

## 3. Definição do problema a ser resolvido

Deve ser gerado 1000 pulsos na maior frequencia possivelno pino de I/O correspondente ao LED1 em C, C+ TIVAWARE e assembly, afim de comparar o tempo de execução destas.

## 4. Especificação da solução
Os requisitos pedidos para a solução foram os seguintes:

**Requisitos Funcionais**
* **RF1:** O sistema deve inicializar e configurar o pino físico de I/O correspondente ao LED1 (Port N, Pino 1) como uma saída digital.
* **RF2:** O sistema deve alternar o estado lógico do pino (Nível Alto seguido de Nível Baixo) para gerar um pulso.
    * **RF2.1:** O sistema deve gerar uma rajada de exatos 1.000 pulsos consecutivos.
* **RF3:** O sistema deve possuir três rotinas distintas de geração dos pulsos
    * **RFN2.1:** Biblioteca de abstração de hardware (TivaWare).
    * **RFN2.2:** Linguagem C com acesso direto e mascarado aos registradores de memória.
    * **RFN2.3:** Linguagem Assembly.

* **RF4:**O sistema deve iniciar um cronômetro/temporizador imediatamente antes do primeiro pulso e pará-lo imediatamente após o milésimo pulso para cada uma das três rotinas.

**Requisitos e Restrições Não Funcionais**
* **RNF 1:** o sistema deve operar na frequência máxima de clock do processador.



## 5. Estudo da plataforma de HW

### PORTN GPIO_DATA (pagina 749)
De acordo com o mapa de memória do microcontrolador TM4C1294, o endereço 0x4006.4000 marca o início do espaço reservado aos registradores do GPIO Porto N via barramento de alta performance (AHB).

Para manipular o pino correspondente ao LED1 (PN1), deve-se atuar sobre o segundo bit do registrador de dados (bit 1), cujo valor posicional é 0x02. No entanto, a arquitetura deste microcontrolador utiliza um método de mascaramento por hardware mapeado no barramento de endereços, utilizando os bits [9:2] como seletores de escrita/leitura.

Este mecanismo exige que o valor da máscara desejada (0x02) seja deslocado duas posições à esquerda (shift left 2), alinhando-o com os bits de endereço que controlam o acesso aos pinos. Matematicamente, o deslocamento de 0x02 << 2 resulta em um offset de 0x008, consolidando o endereço final de acesso ao LED1 em 0x4006.4008.

Essa estrutura de endereçamento força o acesso a endereços múltiplos de 4, respeitando o alinhamento de 32 bits da arquitetura ARM. A principal vantagem desta técnica é a eliminação da operação de Leitura-Modificação-Escrita (Read-Modify-Write): o hardware isola o pino PN1 automaticamente, permitindo alterar seu estado em um único ciclo de instrução, sem o risco de modificar acidentalmente os demais pinos do mesmo porto.



## 6. Estudo da plataforma de SW

### 6.1 Funções utilizadas 
* **SysCtlClockFreqSet()**: configurar o clock principal do sistema, com os parâmetros:
    * **SYSCTL_XTAL_25MHZ**: cristal externo de 25 MHz;
    * **SYSCTL_OSC_MAIN**: fonte base que alimentará os clocks do dispositivo será o Oscilador Principal (MOSC);
    * **SYSCTL_USE_PLL**: ativar e utilizar o circuito PLL (Phase-Locked Loop), para aumentar a frequência de clocks;
    * **SYSCTL_CFG_VCO_480**: Configura o Oscilador Controlado por Tensão (VCO) interno do circuito PLL para gerar uma frequência intermediária bastante alta de 480 MHz;
    * **120000000**: frequência alvo que a CPU e os periféricos devem rodar a 120 MHz.

* **SysCtlPeripheralEnable()**: fornece energia e clock para ativar módulos periféricos específicos, com os parâmetros:
    * **SYSCTL_PERIPH_GPIOA**: habilita o módulo do Port A (onde estão os pinos da UART0);
    * **SYSCTL_PERIPH_UART0**: habilita o módulo de controle da interface serial UART0;
    * **SYSCTL_PERIPH_GPION**: habilita o módulo do Port N (onde está localizado o LED1).

* **SysCtlPeripheralReady()**: verifica e aguarda até que um periférico recém-habilitado esteja fisicamente pronto para ser acessado ou configurado.

* **GPIOPinConfigure()**: mapeia internamente o sinal elétrico de um periférico para um pino físico da placa, com os parâmetros:
    * **GPIO_PA0_U0RX**: direciona a recepção de dados (RX) da UART0 para o pino PA0;
    * **GPIO_PA1_U0TX**: direciona a transmissão de dados (TX) da UART0 para o pino PA1.

* **GPIOPinTypeUART()**: altera a configuração elétrica dos pinos para suportarem comunicação serial, com os parâmetros:
    * **GPIO_PORTA_BASE**: endereço de memória base do Port A;
    * **GPIO_PIN_0 | GPIO_PIN_1**: aplica a configuração simultaneamente aos pinos 0 e 1 do respectivo port.

* **UARTStdioConfig()**: inicializa e configura a biblioteca padrão de I/O serial do TivaWare, com os parâmetros:
    * **0**: indica que a UART de índice zero (UART0) será utilizada;
    * **115200**: define o baud rate (taxa de transmissão) em 115.200 bits por segundo;
    * **clock_atual**: frequência de operação da CPU repassada para garantir o cálculo preciso do sincronismo serial.

* **GPIOPinTypeGPIOOutput()**: configura um pino de propósito geral exclusivamente como saída digital, com os parâmetros:
    * **GPIO_PORTN_BASE**: endereço de memória base do Port N;
    * **GPIO_PIN_1**: pino de índice 1.

* **SysTickPeriodSet()**: define o valor de recarga (duração do ciclo) do temporizador SysTick, com o parâmetro:
    * **0x00FFFFFF**: carrega o contador com o limite máximo de um registrador de 24 bits, equivalendo a 16.777.215 ciclos.

* **SysTickEnable()**: liga o módulo SysTick, fazendo com que ele comece imediatamente a decrementar seu valor a cada ciclo de máquina.

* **SysTickValueGet()**: lê o temporizador do núcleo da CPU em tempo real e retorna o valor exato (em ciclos restantes) no momento da chamada da função.

* **GPIOPinWrite()**: altera o nível lógico de pinos configurados como saída, com os parâmetros:
    * **GPIO_PORTN_BASE**: endereço de memória base do Port N;
    * **GPIO_PIN_1** : máscara determinando qual pino do port será alterado;
    * **GPIO_PIN_1** ou **0** : o dado lógico a ser escrito;

* **Instruções Assembly Utilizadas (Técnica Assembly)**: rotina pura em linguagem de máquina para máxima performance:
    * **MOV**: carrega valores constantes em um registrador ;
    * **LDR**: faz a leitura do endereço longo de memória do registrador de I/O e o salva na CPU;
    * **STR**: escreve o valor lógico contido na CPU direto no endereço de memória;
    * **SUBS**: decrementa o registrador e atualiza a *flag* ex: zero;
    * **BNE**: (Branch if Not Equal) salta de volta para o início do loop caso flag Z == 0.
    * **BX LR**: executa o retorno da função Assembly para a função `main()` nativa do C.

* **UARTprintf()**: formata e envia uma string de texto através do cabo USB via protocolo UART, exibindo o resultado do cronômetro para o usuário no terminal serial.

## 7. Projeto (design) da solução

**Fluxo de Execução:**
`INÍCIO CLOCK` -> `INÍCIO UART` -> `COMEÇO CONTADOR DE CICLO` -> `Código Tivaware` -> `FIM CONTADOR DE CICLO` -> `COMEÇO CONTADOR DE CICLO` -> `Código C` -> `FIM CONTADOR DE CICLO`-> `COMEÇO CONTADOR DE CICLO` -> `Código Assembly inline` -> `FIM CONTADOR DE CICLO`-> `COMEÇO CONTADOR DE CICLO` -> `Código THOUSAND_PULSE` -> `FIM CONTADOR DE CICLO` -> `PRINTS`

**THOUSAND_PULSE:**
 `INICIA CONTADOR = 1000` -> `PEGA ENDEREÇO 0x40064008 ` -> `Looping alterando o endereço` -> `FIM`



## 8. Configuração do projeto na IDE (Keil uVision)

O Target do Keil foi configurado selecionando o microcontrolador **TM4C1294NCPDT**. Foram inclusos os arquivos `uartstdio.h` e `uartstdio.c` para o terminal serial funcionar. Foi inclusa também a biblioteca `driverlib.lib` para as funções que dependiam dela.

Foi necessário apontar os diretórios de inclusão (Include Paths) para as seguintes pastas:
* `..\..\..\TivaWare_C_Series-2.2.0.295\utils`
* `..\..\..\TivaWare_C_Series-2.2.0.295`
* `..\..\..\TivaWare_C_Series-2.2.0.295\driverlib`
* `.\src_others`

  A organização dos arquivos do projeto segue a seguinte estrutura:

- main.c: responsável pela execução dos testes e chamada da função em assembly.
- thousands_pulse.s: implementação da função thousands_pulse em assembly.

## 9. Teste e depuração

### Resultado

É visto pela quantidade de clocks que:

* **ciclos_TivaWare**: Cada chamada obriga a CPU a ramificar o código, empilhar registradores, executar a função e retornar, o que consome dezenas de ciclos por iteração.Eles são escritos para lidar com casos genéricos e operações de rotina de forma fácil de entender, e não customizados para os requisitos específicos da aplicação, que seria o papel do Assembly.
* **C**: O uso do modelo de acesso direto a registradores geralmente resulta em um código menor e mais eficiente do que o uso do modelo de driver de software;
* **ciclos_Assembly_inline**: Perdeu em eficiencia para o código em C, talvez pela otimização do código em C feito pelo compilador.
* **ciclos_Assembly**: Perdeu em eficiencia para o código em ciclos_Assembly_inline, pois precisa entrar aramazenar todos os registradores, executar e voltar para o código principal.

```txt
ciclos_TivaWare = 21027
ciclos_C = 5014
ciclos_Assembly_inline = 5030
ciclos_Assembly = 5042
```


## 10. Referências


* TEXAS INSTRUMENTS. Tiva™ TM4C1294NCPDT Microcontroller: Data Sheet. Literature Number: SPMS433B. Austin, TX: Texas Instruments Incorporated, 2014
* TEXAS INSTRUMENTS. TivaWare™ Peripheral Driver Library: User's Guide. Literature Number: SPMU298E. Austin, TX: Texas Instruments Incorporated, 2020
