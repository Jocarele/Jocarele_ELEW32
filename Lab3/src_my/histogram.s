	PRESERVE8 
	THUMB
		
	AREA	|.text|, CODE, READONLY, ALIGN=3
	
	EXPORT EightBitHistogram
	

	; Calcula o histograma de uma imagem 8 bits
	; Entrada:
	;   R0 = largura (width)
	;   R1 = altura (height)
	;   R2 = ponteiro para imagem
	;   R3 = ponteiro para histograma (256 posições)
	; Saída:
	;   R0 = total de pixels (ou 0 em caso de erro)

EightBitHistogram PROC
	PUSH {R4-R7, LR}
	; Calcula total de pixels (width * height)
	MUL R4, R0, R1

    ; limite 64K
    LDR R5, =65536
    CMP R4, R5
    BHI erro

    ; zerar histograma
    MOV R5, #0

zera
    CMP R5, #256
    BEQ inicia


    MOV R0, #0
	; address = R3 + R5<<1  R5 = indice * 2
    STRH R0, [R3,R5,LSL #1]

    ADD R5, R5, #1
    B zera

inicia
    MOV R5, R2
    MOV R6, #0

; Percorre todos os pixels da imagem
loop
    CMP R6, R4
    BGE fim

; Lê próximo pixel da imagem e avança ponteiro
    LDRB R7, [R5], #1

; Calcula endereço do histograma (pixel * 2)
	LDRH R1, [R3,R7,LSL #1]
	ADD R1, R1, #1
	STRH R1, [R3,R7, LSL #1]

; Incrementa contador do valor do pixel

    ADD R6, R6, #1
    B loop

fim
	; Retorna total de pixels processados
    MOV R0, R4
    POP {R4-R7, LR}
    BX LR

erro
	; Retorna 0 em caso de erro (imagem muito grande)
    MOV R0, #0
    POP {R4-R7, LR}
    BX LR
	ENDP
		
	END