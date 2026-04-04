# Relatório do Lab3

**Nomes:**
* João Lucas Marques Camilo
* Bruno Ribeiro Basilio

---

## 1. Introdução

Este relatório descreve a interação com código assembly que será uma rotina dentro de um programa em C ou C++. A rotina deve gerar um histograma de uma imagem em tons de cinza. O histograma deve ser apresentado textualmente em um terminal TTY.

## 2. Planejamento do processo de desenvolvimento

* Estudo sobre instruções assembly.
* Revisão Lab2, sobre inicialização terminal TTY.
* Pesquisa sobre o contador DWT.
* Testes, depuração e sincronização com o repositório GitHub.

## 3. Definição do problema a ser resolvido

Elaborar uma rotina em assembly que será chamada de um programa em C ou C++. A rotina deve gerar um histograma de uma imagem em tons de cinza. O histograma deve ser apresentado textualmente em um terminal TTY.

## 4. Especificação da solução
> **TODO:** Fazer requisitos.

**Requisitos Funcionais**
* **RF1:** [TODO]
    * **RF1.1:** [TODO]
* **RF2:** [TODO]
* **RF3:** [TODO]

**Requisitos e Restrições Não Funcionais**
* **RNF 1:** [TODO]
* **RNF 2:** [TODO]
* **RNF 3:** [TODO]
* **RNF 4:** [TODO]
* **RNF 5:** [TODO]

## 5. Estudo da plataforma de HW

### 5.1 Contador DWT
O contador DWT provê *watchpoints, data tracing e system profiling* para o processador. Possui até quatro comparadores, e foi utilizado o primeiro comparador, `DWT_COMP0`, que consegue comparar com o `CYCCNT` (*clock cycle counter*) a fim de conseguir a resolução de 1 clock. 

Para iniciar o contador, deve-se setar o `CYCCNT`, dando início ao contador de ciclos de 32 bits. Em *overflow*, o contador é resetado.

### 5.2 Tamanho Stack

A placa **TM4C1294NCPDT** tem configurado no keil o tamanho de pilha *stack* de 512 bytes, como demonstrado na imagem 1. Então, para que eu tenha um vetores *uint16* de 256 de tamanho , é necessário alocar o outro vetor em outra parte da memória, colocando por exemplo, o atributo **static**, pois o vetor ocupa no total 512 bytes de memória.

![Imagem 1 - stack_size](./assets/stack_size.png)
## 6. Estudo da plataforma de SW

### 6.1 Assembly 

#### 6.1.1 Chamada da rotina em assembly
De acordo com a documentação, para juntar dois módulos separados através do *GNU assembler*, é necessário usar o atributo `extern` no código em C para chamar uma função em assembly. É possível também fazer uma função assembly no código C utilizando o atributo `asm`.

#### 6.1.2 Argumentos
A tabela 1 mostra os registradores que são passados como argumentos: `R0`-`R3` e `R12`. Caso seja necessário utilizar mais registradores, é obrigatório dar *push* na pilha (stack) no começo da função para armazenar seus valores originais, e *pop* ao sair da função.

![Tabela 1 - Registradores](./assets/registradores.png)

Como o exercício requer calcular o histograma de uma imagem, teremos como parâmetros da função:

```assembly	
; Entrada:
;   R0 = largura (width) (int 4 bytes)
;   R1 = altura (height) (int 4 bytes)
;   R2 = ponteiro para a imagem (ponteiro para o vetor 4 bytes)
;   R3 = ponteiro para o histograma (256 posições) (ponteiro para o vetor 4 bytes)
; Saída:
;   R0 = total de pixels (ou 0 em caso de erro) (int 4 bytes)
```

#### 6.1.3 Comandos Assembly
* **`PUSH`**: Coloca os registradores na pilha.
* **`POP`**: Retira os registradores da pilha, sempre respeitando a ordem.
* **`LDR`**: Armazena valor num registrador de 32 bits caso seja passada uma constante. Também pode ler um valor da memória caso seja passada uma posição de memória entre colchetes.
* **`CMP`**: Compara se dois valores/registradores são iguais (atualiza a flag Z para 1 se for verdade).
* **`BHI`**: Pula (Branch) para uma label caso a flag de *Higher* (maior que unsigned) estiver ativa.
* **`MOV`**: Move um valor direto ou de outro registrador para um registrador destino.
* **`BEQ`**: Pula (Branch) para a label se a flag Z = 1 (valores comparados eram iguais).
* **`LSL`**: Empurra os bits para a esquerda (*Logical Shift Left*), equivalente a multiplicar por 2.
* **`ADD`**: Faz operação de soma.
* **`STRH`**: Guarda o valor do registrador em uma posição de memória, determinada pelos colchetes. O 'H' vem de *Halfword* (2 bytes).

## 7. Projeto (design) da solução

**Fluxo de Execução:**
`INÍCIO CLOCK` -> `INÍCIO UART` -> `COMEÇO CONTADOR DE CICLO` -> `EightBitHistogram` -> `FIM CONTADOR DE CICLO` -> `PRINTS`

**EightBitHistogram:**
`SALVA REGISTRADORES PILHA` -> `ZERA HISTOGRAMA` -> `CONTA PIXELS` -> `RETURN`

> **TODO:** Verificar se precisa detalhar o looping do zeramento e da contagem no diagrama UML.FAZER UML

## 8. Configuração do projeto na IDE (Keil uVision)

O Target do Keil foi configurado selecionando o microcontrolador **TM4C1294NCPDT**. Foram inclusos os arquivos `uartstdio.h` e `uartstdio.c` para o terminal serial funcionar. Foi inclusa também a biblioteca `driverlib.lib` para as funções que dependiam dela.

Foi necessário apontar os diretórios de inclusão (Include Paths) para as seguintes pastas:
* `C:\ti\TivaWare_C_Series-2.2.0.295\utils`
* `C:\ti\TivaWare_C_Series-2.2.0.295`
* `C:\ti\TivaWare_C_Series-2.2.0.295\driverlib`
* `.\src_others`

## 9. Teste e depuração

### 9.1 Teste histograma
Para teste foi feito o seguinte código em C:

```C
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

```

Basicamente, caso haja diferença entre os histogramas, deve ser printado no terminal do PUTTY, algo que não aconteceu:

```txt
Teste com imagem do professor
Total: 19200
Clocks: 347958

Debug finalizado.

```
### 9.2 teste clock
> **TODO:** Eu to sem ideia, qualquer coisa apagar tópico.


[TODO] 

## 10. Referências

* [Mixing C and assembly code](https://developer.arm.com/documentation/den0013/0400/Application-Binary-Interfaces/Mixing-C-and-assembly-code)
* [Calling between C/C++ and ARM assembly language](https://developer.arm.com/documentation/dui0056/d/mixing-c--c----and-assembly-language/calling-between-c--c----and-arm-assembly-language/examples)
* [Data Watchpoint and Trace Unit](https://developer.arm.com/documentation/ddi0439/b/Data-Watchpoint-and-Trace-Unit?lang=en)
* [Control register, DWT_CTRL](https://developer.arm.com/documentation/ddi0403/d/Debug-Architecture/ARMv7-M-Debug/The-Data-Watchpoint-and-Trace-unit/Control-register--DWT-CTRL)