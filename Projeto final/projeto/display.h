#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "grlib/grlib.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"

// ==========================================================
// DEFINIÇÕES E ESTADOS (Exportados para a main.c)
// ==========================================================
typedef enum {
    TELA_PRINCIPAL = 0, // Modo de Visualização
    MENU_PRINCIPAL,     // Modo de Configuração
    MENU_TAXA,
    MENU_VDIV,
    MENU_HDIV,
    MENU_NIVEL_TRIG,
    MENU_BORDA_TRIG,
		MENU_MODO_SINGLE
		
} EstadoTela;

typedef struct {
    int taxa_khz;
    int v_div;      // mV/div
    int h_div;      // ms/div
    int modo_single;
    int nivel;      // mV
		int borda_trigger;
} Onda_conf;

// ==========================================================
// VARIÁVEIS GLOBAIS EXPORTADAS
// ==========================================================
extern tContext grContext; // A main precisa conhecer o pincel
extern volatile EstadoTela estado_atual;

extern int menu_selecionado; 
extern int indice_taxa;      
extern int indice_vdiv;      
extern int indice_hdiv;      
extern int indice_nivel_trig;
extern int indice_borda_trig;
extern int indice_modo_single;

extern const int valores_taxa[];
extern const int valores_vdiv[];
extern const int valores_hdiv[];
extern const int valores_nivel[];
extern const char* str_borda_trig[];
extern const char* titulos_menu[];
extern const char* str_modo_single[];
extern Onda_conf onda;



// ==========================================================
// PROTÓTIPOS DAS FUNÇÕES
// ==========================================================
void DisplaySetup(void);
void DesenharMenuPrincipal(void);
void DesenharSubMenu(const char* titulo,  int valor_atual,const char* unidade); 
void DesenharOsciloscopio(tContext *pContext, uint32_t *buffer, uint16_t num_pontos, 
                          const char* texto_vdiv, const char* texto_tdiv, bool is_running);
void FormatarValor(char* buf, int val, const char* unidade);
static int16_t ADCParaYPixel(uint32_t valor_adc);

#endif // DISPLAY_H