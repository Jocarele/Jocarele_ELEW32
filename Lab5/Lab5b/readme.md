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

## 5. Processo de Inicialização

A inicialização do sistema ocorre em etapas sucessivas, desde o Reset do hardware até a execução da thread:

1. **Vetor de Reset (`startup_TM4C129.s`):** Ao alimentar a placa, a CPU busca o endereço da função `Reset_Handler`. Este código em Assembly faz configurações críticas de hardware, como a habilitação da FPU (Unidade de Ponto Flutuante).
2. **Ambiente C (`__main`):** O controle passa para a biblioteca de tempo de execução C, que copia dados da Flash para a RAM e limpa a seção `.bss`. Em seguida, a função `main()` é chamada.
3. **Setup de Hardware e Kernel (`main.c`):** - O clock é configurado para **120 MHz** via `SysCtlClockFreqSet`.
   - A Porta N (LED) é habilitada.
   - O sistema chama `tx_kernel_enter()`, que inicia o processo de "subida" do RTOS.
4. **Baixo Nível (`tx_initialize_low_level.s`):** O ThreadX assume o controle e configura o **SysTick**. Com a CPU a 120 MHz, o registrador de reload é configurado para gerar interrupções a **100 Hz** (10ms por tick). As prioridades de interrupção (SVC e PendSV) são ajustadas para permitir a troca de contexto segura.
5. **Definição da Aplicação (`tx_application_define`):** O kernel chama esta função para que o usuário crie seus recursos. Aqui criamos a `thread_0` e alocamos sua pilha (*stack*).
6. **Agendamento (`_tx_thread_schedule`):** Após as definições, o escalonador é iniciado. Ele detecta que a `thread_0` está pronta, restaura seu contexto nos registradores da CPU e inicia a execução da função `thread_0_entry`.

---

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