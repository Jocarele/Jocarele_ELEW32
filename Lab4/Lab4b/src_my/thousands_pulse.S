	PRESERVE8 
	THUMB
		
	AREA	|.text|, CODE, READONLY, ALIGN=3
	
	EXPORT thousand_pulse

thousand_pulse PROC
	mov r0, #1000            ; R0 = contador do loop (1000)
	ldr r1, =0x40064008      ; R1 = endereço de memória do Port N, Pino 1
	mov r2, #2                ; R2 = valor ALTO (0x02)
	mov r3, #0               ; R3 = valor BAIXO (0x00)
loop_pulso                   ;
	str r2, [r1]             ; Escreve ALTO no endereço do registrador
	str r3, [r1]             ; Escreve BAIXO no endereço do registrador
	subs r0, r0, #1         ; Decrementa o contador (R0 = R0 - 1)
	bne loop_pulso 
	BX LR
	
	ENDP
		
	END