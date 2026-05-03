# Relatório do Lab5b - ThreadX

**Nomes:**
* João Lucas Marques Camilo
* Bruno Ribeiro Basilio

---

## 1. Introdução

Este relatório descreve a implementação, configuração e validação do Sistema Operacional de Tempo Real (RTOS) **ThreadX** na plataforma de hardware Tiva TM4C1294NCPDT. O principal foco do projeto é adequar a temporização do kernel e do hardware, consolidando os conhecimentos sobre os processos de inicialização de um sistema multitarefa.

## 2. Planejamento do processo de desenvolvimento

* Criação das bibliotecas nativas e adaptação dos *Include Paths* na IDE Keil.
* Estudo do comportamento das threads e objetos no código de demonstração.
* Simplificação da aplicação para uma única thread de usuário, com foco em testes de temporização.
* Configuração do clock da placa para 120 MHz através da biblioteca TivaWare no `main()`.
* Análise e configuração do contador `SysTick` via arquivo de baixo nível (`tx_initialize_low_level.s`).
* Validação de temporização utilizando o LED 1 da placa (Porta N, Pino 0).

## 3. Definição do problema a ser resolvido

O objetivo central foi portar e calibrar o RTOS ThreadX. Isso exigiu garantir que a frequência da CPU (120 MHz) e os ciclos do `SysTick` estivessem em sintonia com os "ticks" do sistema operacional. Para validar essa temporização, a aplicação foi configurada com uma única thread para piscar o LED em um período de 2 segundos (1 segundo aceso, 1 segundo apagado).

## 4. Análise do Código e Estruturas

Abaixo estão as tabelas referentes ao projeto desenvolvido, mapeando a thread ativa e os objetos de sistema criados durante a inicialização.

### Tabela 1: Threads da Aplicação

| Thread Name | Entry function         | Stack size | Priority | Auto start | Time slicing |
| ----------- | ---------------------- | ---------- | -------- | ---------- | ------------ |
| thread 0    | thread_0_entry         | 1024       | 1        | yes        | no           |

### Tabela 2: Objetos de Sistema

| Name           | Control structure | Size/Count   | Location                                      |
| -------------- | ----------------- | ------------ | --------------------------------------------- |
| byte pool 0    | byte_pool_0       | 9120 bytes   | Variável global em seção `.bss`               |


---

## 5. Descrição Detalhada do Processo de Inicialização

O processo de inicialização do Lab5b é dividido em etapas que preparam o hardware, o ambiente da linguagem C e, por fim, o kernel do RTOS. Abaixo, detalha-se o caminho percorrido desde o Reset até a execução da thread do LED.

### Passo 1: startup_TM4C129.s
Assim que a placa é energizada ou o botão de Reset é pressionado, o processador Cortex-M4 executa uma rotina automática de hardware: ele lê os dois primeiros endereços da memória Flash. O primeiro fornece o endereço da pilha (Stack Pointer) e o segundo o endereço da primeira instrução a ser executada, chamada de `Reset_Handler`. 

### Passo 2: __main
Ainda no arquivo de startup, após o `Reset_Handler`, o processador salta para a rotina `__main` que copia as variáveis que têm valores iniciais da Flash para a RAM e zera o restante da memória.

### Passo 3: main.c
Já dentro do `main()`, é realizado a configuração de clock utilizando a função `SysCtlClockFreqSet` da TivaWare para configurar o PLL e o oscilador externo para que a CPU rode a **120 MHz**. 

### Passo 4: tx_initialize_low_level.s
Ao chamar `tx_kernel_enter()`, o ThreadX assume o controle. É necessário configurar o **SysTick** para o arquivo em assembly, o qual o RTOS usa  para calcular quanto tempo o cronômetro deve contar para apitar exatamente 100 vezes por segundo (100 Hz). 

### Passo 5: tx_application_define
 O sistema executa `tx_application_define`, onde são criados:
1.  **Byte Pool**: que é a reserva de memória RAM para as futuras aplicações (no caso a thread0)
2.  **thread_0**: definindo sua função de entrada (`thread_0_entry`) e sua prioridade. O estado da thread passa a ser "Ready" (Pronta).

### Passo 6: O Início do Escalonamento (Multitarefa)
Por fim, o kernel termina sua inicialização e inicia o **Scheduler (Escalonador)**, o responsavel por identificar a prontidão da `thread_0`, carregando seu contexto (registradores do processador) e salta para o código que faz o LED piscar. 

## 6. Teste de Temporização e Resultados

Para validar o funcionamento, a `thread_0` utiliza a função `tx_thread_sleep(100)`. Dado que o sistema foi calibrado para 100 ticks por segundo, esse comando gera um atraso de exatamente **1 segundo**.

**Metodologia de Validação:**
O ciclo de pisca consiste em 1 segundo ligado e 1 segundo desligado (período total de 2s). 
Utilizamos um cronômetro para medir o tempo de **15 piscadas** (momentos em que o LED acende).

* **Cálculo Esperado:** O momento em que o LED acende pela 15ª vez ocorre após 15 intervalos de 2 segundos.
* **Resultado:** 15 * 2 = **30 segundos**.
* **Observação:** O cronômetro marcou exatamente 30 segundos na 15ª piscada, confirmando que o clock de 120 MHz e a ISR do SysTick estão perfeitamente sincronizados com o RTOS.

## 7. Referências

* TEXAS INSTRUMENTS. Tiva™ TM4C1294NCPDT Microcontroller: Data Sheet.
* MICROSOFT. Azure RTOS ThreadX User Guide.
* UTFPR. Roteiro de Laboratório 5 - Prof. Douglas Renaux.