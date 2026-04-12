## Relatório do Lab4a

## Nomes:

João Lucas Marques Camilo
Bruno Ribeiro Basilio

## 1. Introdução

Este relatório apresenta o desenvolvimento de um sistema embarcado capaz de realizar a leitura de um joystick (eixos X, Y e botão), enviar os valores obtidos via comunicação serial UART e controlar um LED RGB com base nesses dados.

O objetivo do experimento é integrar diferentes periféricos do microcontrolador TM4C1294NCPDT, utilizando módulos como GPIO, ADC e UART, além de explorar a interação com a placa BoosterPack MKII.

## 2. Planejamento do processo de desenvolvimento

Inicialmente foi realizada uma revisão sobre o funcionamento do módulo ADC e da comunicação UART no microcontrolador. Em seguida, foi estudada a forma de leitura de sinais analógicos provenientes do joystick.

Após isso, foi feita a implementação da leitura dos eixos X e Y, bem como do botão. Em paralelo, foi configurada a UART para envio dos dados ao terminal serial.

Por fim, foi implementado o controle do LED RGB e realizados testes práticos, seguidos de depuração e análise dos resultados obtidos.

## 3. Definição do problema a ser resolvido

O sistema deve:

Ler continuamente os valores do joystick (horizontal, vertical e botão)
Enviar essas informações pela UART0 a cada aproximadamente 200 ms
Controlar um LED RGB de acordo com a posição do joystick.

##4. Especificação da solução

**Requisitos Funcionais:**

**RF1:** Configurar a UART0 para comunicação serial a 115200 bps (8N1).

**RF2:** Ler os valores analógicos dos eixos X e Y utilizando o ADC0.

**RF3:** Ler o estado do botão do joystick como entrada digital.

**RF4:** Enviar via UART os dados no formato:
X:valor Y:valor BTN:estado

**RF5:** Atualizar os dados aproximadamente a cada 200 ms.

**RF6:** Controlar o LED RGB com base nos valores do joystick:

Eixo X → vermelho e azul
Eixo Y → verde
Botão pressionado → LED desligado

**Requisitos Não Funcionais:**

**RNF1:** Operar com clock de 120 MHz.

**RNF2:** Utilizar a biblioteca TivaWare para configuração dos periféricos.

## 5.Estudo da plataforma de HW
### LED RGB

O LED RGB utilizado está presente na placa BoosterPack MKII e é controlado por sinais digitais provenientes do microcontrolador.

Cada cor do LED (vermelho, verde e azul) é acionada individualmente por um pino GPIO, permitindo a combinação de cores conforme os sinais aplicados.

De acordo com o mapeamento da BoosterPack MKII, os sinais do LED RGB estão conectados aos seguintes pinos do microcontrolador:

J37 → PF2 → LED vermelho
J38 → PF3 → LED verde
J39 → PG0 → LED azul

Inicialmente, o controle do LED não apresentou o comportamento esperado. A partir da análise do diagrama de pinagem da BoosterPack, foi possível identificar o mapeamento correto dos pinos.

### Joystick

O joystick é composto por dois potenciômetros (eixos X e Y) e um botão digital.

A leitura dos eixos foi realizada por meio do ADC0:

* PE4 → ADC Channel 9 → eixo X

* PE3 → ADC Channel 0 → eixo Y

Os valores obtidos variam de 0 a 4095, devido à resolução de 12 bits do ADC.

O botão foi configurado como entrada digital:

PC6 → entrada com resistor de pull-up interno

**Mapeamento Cortex-M ↔ BoosterPack**

Com a BoosterPack MKII conectada na posição BoosterPack 1, os sinais são encaminhados através dos headers da placa Tiva até o microcontrolador.

Mapeamento Cortex-M ↔ BoosterPack
| Periférico   | Pino no TM4C1294 |
|-------------|------------------|
| Joystick X  | PE4              |
| Joystick Y  | PE3              |
| Botão       | PC6              |
| LED Vermelho| PF2              |
| LED Verde   | PF3              |
| LED Azul    | PG0              |

**Descrição do mapeamento de pinos**

A tabela apresentada relaciona os sinais físicos da BoosterPack MKII com os pinos correspondentes do microcontrolador TM4C1294.

A identificação dos sinais na BoosterPack é feita por meio dos conectores (J1, J2, J3 e J4), onde cada pino possui uma função específica, como entrada analógica, saída digital ou comunicação com periféricos.

Esses sinais são encaminhados diretamente para os pinos do microcontrolador através dos headers da placa Tiva, permitindo que o software acesse os dispositivos conectados à BoosterPack.

No caso do LED RGB, por exemplo, os sinais identificados como J37, J38 e J39 correspondem, respectivamente, aos pinos PF2, PF3 e PG0 do microcontrolador. Essa relação é fundamental para garantir que o controle implementado em software atue corretamente sobre o hardware.

Dessa forma, a correta interpretação do mapeamento de pinos é essencial para a integração entre a aplicação desenvolvida e os periféricos disponíveis na BoosterPack.

**Configuração dos pinos**

**UART**
PA0 → RX
PA1 → TX
**ADC**
PE3 → entrada analógica
PE4 → entrada analógica
**Botão**
PC6 → entrada digital com pull-up
**LED RGB**
Configurado como saída digital (GPIO)

### Diagrama em blocos

O sistema pode ser dividido em três partes principais: computador, placa Tiva e BoosterPack.

        +---------------------------+
        |        Computador         |
        |    (Terminal Serial)      |
        +-------------+-------------+
                      |
                   UART0
                      |
        +-------------+-------------+
        |        Placa Tiva C       |
        |       (TM4C1294)          |
        |                           |
        |  +---------------------+  |
        |  |  Cortex-M4F (CPU)   |  |
        |  +---------------------+  |
        |  | ADC0 | GPIO | UART0 |  |
        |  +---------------------+  |
        +-------------+-------------+
                      |
              Headers BoosterPack
                      |
        +-------------+-------------+
        |      BoosterPack MKII     |
        |                           |
        |  Joystick (ADC X/Y)       |
        |  Botão (GPIO)             |
        |  LED RGB (GPIO)           |
        +---------------------------+


## 6. Estudo da plataforma de SW
### 6.1 Funções utilizadas

**SysCtlClockFreqSet():** Configura o clock principal do sistema para 120 MHz.

**SysCtlPeripheralEnable():** Habilita os periféricos utilizados (GPIO, ADC e UART).

**SysCtlPeripheralReady():** Verifica se o periférico já está pronto para uso.

**GPIOPinConfigure():** Associa funções periféricas aos pinos físicos.

**GPIOPinTypeUART():** Configura pinos para comunicação serial.

**UARTStdioConfig():** Inicializa a UART com o baud rate desejado.

**GPIOPinTypeADC():** Configura pinos como entradas analógicas.

**ADCSequenceConfigure():** Configura o sequenciador do ADC.

**ADCSequenceStepConfigure():** Define os canais que serão lidos.

**ADCProcessorTrigger():** Inicia a conversão ADC via software.

**ADCSequenceDataGet():** Obtém os valores convertidos.

**ADCIntClear():** Limpa a flag de interrupção do ADC.

**GPIOPinTypeGPIOInput():** Configura um pino como entrada digital.

**GPIOPadConfigSet():** Configura o resistor de pull-up interno.

**GPIOPinTypeGPIOOutput():** Configura um pino como saída digital.

**GPIOPinWrite():** Escreve o valor lógico nos pinos.

**SysCtlDelay():** Gera um atraso aproximado.

**UARTprintf():** Envia dados formatados pela UART.

## 7.Projeto (design) da solução

### Fluxo de execução:

INÍCIO → Configuração do clock → Configuração UART → Configuração ADC → Configuração GPIO → Loop principal

**No loop principal:**

**1.** Leitura dos valores do ADC

**2.** Leitura do botão

**3.** Envio dos dados via UART

**4.** Cálculo da cor do LED

**5.** Escrita nos pinos de saída

**6.** Atraso de aproximadamente 200 ms

## 8. Configuração do projeto na IDE (Keil uVision)
O Target do Keil foi configurado selecionando o microcontrolador TM4C1294NCPDT. Foram inclusos os arquivos `uartstdio.h` e `uartstdio.c` para o funcionamento do terminal serial. Também foi utilizada a biblioteca driverlib.lib para acesso às funções de abstração de hardware.

Foi necessário configurar os diretórios de inclusão (Include Paths) para as seguintes pastas:

* ..\..\..\TivaWare_C_Series-2.2.0.295\utils
* ..\..\..\TivaWare_C_Series-2.2.0.295
* ..\..\..\TivaWare_C_Series-2.2.0.295\driverlib
* .\src_others

A organização dos arquivos do projeto segue a seguinte estrutura:

main.c: responsável pela configuração dos periféricos, leitura do joystick, controle do LED RGB e envio dos dados via UART.

## 9. Teste e depuração
### Resultados

A leitura do joystick apresentou valores coerentes, com aproximadamente 2000 na posição central e valores próximos aos extremos (0 e 4095) quando deslocado.

A comunicação UART funcionou corretamente, exibindo os dados no terminal serial conforme esperado.

O botão também respondeu corretamente às interações.

Quando o joystick foi deslocado para uma das extremidades, observou-se que os valores atingem o limite máximo do conversor A/D, conforme mostrado abaixo:

X:4095 Y:4095 BTN:0

Esse comportamento indica que ambos os eixos estão operando corretamente e atingindo o valor máximo esperado (4095), correspondente à resolução de 12 bits do ADC.

Ao movimentar o joystick parcialmente, foram obtidos valores intermediários, como:

* X:4095 Y:3666
* X:4095 Y:3697
* X:4095 Y:3709
* X:3223 Y:3006

Esses valores demonstram a variação contínua e proporcional dos sinais analógicos, confirmando o correto funcionamento da leitura dos eixos X e Y.

Além disso, quando o joystick foi posicionado próximo ao centro, foram observados valores intermediários mais baixos, como:

* X:1741 Y:0
* X:1665 Y:0
* X:1648 Y:0
* X:1610 Y:0

Apesar de haver pequenas variações (ruído), os valores se mantêm relativamente estáveis, o que é esperado em leituras analógicas.

Quando o botão é pressionado, o valor de BTN passa para 1, retornando a 0 quando liberado.

### Análise do problema

Durante os testes iniciais, o LED RGB não apresentou funcionamento conforme esperado, mesmo com a lógica de controle implementada.

A partir da análise do diagrama de pinagem da BoosterPack MKII, foi possível identificar a correspondência entre os sinais do LED RGB e os pinos do microcontrolador.

Verificou-se que o LED RGB está conectado aos seguintes pinos:

PF2 (vermelho)
PF3 (verde)
PG0 (azul)

Com base nessa identificação, foi possível compreender o comportamento observado e alinhar o controle do software ao mapeamento físico da BoosterPack.

## 10. Referências

TEXAS INSTRUMENTS. Tiva™ TM4C1294NCPDT Microcontroller Data Sheet.

TEXAS INSTRUMENTS. TivaWare™ Peripheral Driver Library User’s Guide.

Material da disciplina de Sistemas Embarcados – UTFPR.
