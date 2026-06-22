#include "display.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"


#define TELA_LARGURA 128
#define TELA_ALTURA  128

tContext grContext;

volatile EstadoTela estado_atual = TELA_PRINCIPAL;

int menu_selecionado = 0; 
int indice_taxa = 1;      
int indice_vdiv = 1;      
int indice_hdiv = 1;      
int indice_nivel_trig = 1;
int indice_borda_trig = 0;

const int valores_taxa[] = {1, 5, 10};
const int valores_vdiv[] = {5, 10, 20};
const int valores_hdiv[] = {1, 5, 10};
const int valores_nivel[] = {10, 15, 20}; 

const char* titulos_menu[] = {"Taxa (kHz)", "Escala V (V)", "Escala H (ms)", "Nivel (V)", "Borda"};
const char* str_borda_trig[] = {"Subida", "Descida"};



// SETUP DISPLAY
void DisplaySetup(void) {
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    GrContextInit(&grContext, &g_sCrystalfontz128x128);
    //GrFlush(&grContext);
    GrContextFontSet(&grContext, &g_sFontFixed6x8);
}


void FormatarValor(char* buf, int val, const char* unidade, bool decimais) {
    int i = 0;
    if(decimais) {
        buf[i++] = (val / 10) + '0';
        buf[i++] = '.';
        buf[i++] = (val % 10) + '0';
    } else {
        buf[i++] = (val / 10) + '0'; // Exemplo simples
    }
    buf[i++] = ' ';
    while(*unidade) buf[i++] = *unidade++;
    buf[i] = '\0';
}

void DesenharMenuPrincipal(void) {
    tRectangle rectFundo = {0, 0, 127, 127};
    GrContextForegroundSet(&grContext, ClrBlack);
    GrRectFill(&grContext, &rectFundo);

    // Cabeçalho
    GrContextForegroundSet(&grContext, ClrWhite);
    GrContextFontSet(&grContext, g_psFontFixed6x8);
    GrStringDrawCentered(&grContext, "CONFIGURACOES", -1, 64, 8, false);
    GrLineDraw(&grContext, 0, 16, 127, 16);

    for(int i = 0; i < 5; i++) {
        int y_pos = 22 + (i * 18);

        if (menu_selecionado == i) {
            tRectangle rectSel = {2, y_pos - 2, 125, y_pos + 10};
            GrContextForegroundSet(&grContext, ClrDarkBlue);
            GrRectFill(&grContext, &rectSel);
            GrContextForegroundSet(&grContext, ClrWhite);
            GrStringDraw(&grContext, ">", -1, 4, y_pos, false);
        } else {
            GrContextForegroundSet(&grContext, ClrGray);
        }
        GrStringDraw(&grContext, titulos_menu[i], -1, 14, y_pos, false);
    }
    GrFlush(&grContext);
}

void DesenharSubMenu(const char* titulo, int valor, const char* unidade, bool dec) {
    char texto_valor[20];
    tRectangle rectFundo = {0, 0, 127, 127};

    GrContextForegroundSet(&grContext, ClrBlack);
    GrRectFill(&grContext, &rectFundo);

    // Título
    GrContextForegroundSet(&grContext, ClrWhite);
    GrStringDrawCentered(&grContext, titulo, -1, 64, 10, false);
    GrLineDraw(&grContext, 0, 20, 127, 20);

    // Substituímos o sprintf por esta chamada leve
    FormatarValor(texto_valor, valor, unidade, dec);

    // Valor Central
    GrStringDrawCentered(&grContext, "<", -1, 20, 64, false);
    GrStringDrawCentered(&grContext, texto_valor, -1, 64, 64, false);
    GrStringDrawCentered(&grContext, ">", -1, 108, 64, false);

    // Rodapé
    GrLineDraw(&grContext, 0, 105, 127, 105);
    GrStringDraw(&grContext, "Voltar", -1, 5, 115, false);
    GrStringDraw(&grContext, "Sel", -1, 95, 115, false); // Encurtei para caber melhor
    GrFlush(&grContext);
}

void DesenharOsciloscopio(tContext *pContext, uint32_t *buffer, uint16_t num_pontos, 
                          const char* texto_vdiv, const char* texto_tdiv, bool is_running)
{
    tRectangle rectTela;
    rectTela.i16XMin = 0;
    rectTela.i16YMin = 0;
    rectTela.i16XMax = TELA_LARGURA - 1;
    rectTela.i16YMax = TELA_ALTURA - 1;

    // Fundo Preto
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &rectTela);

	
    // grade
    GrContextForegroundSet(pContext, ClrDarkSlateGray);
	//TODO: V_DIV
    for (int i = 16; i < TELA_LARGURA; i += 16) {
        GrLineDraw(pContext, 0, i, TELA_LARGURA, i);
    }
	//TODO: T_DIV
	for (int i = 16; i < TELA_ALTURA; i += 16) {
        GrLineDraw(pContext, i, 0, i, TELA_ALTURA);
    }

    // texto interface
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextFontSet(pContext, g_psFontFixed6x8);

    // Canto superior esquerdo: Canal
    GrStringDraw(pContext, "CH1", -1, 2, 2, true);

    // run/stop (one-shot feacture)
    if (is_running) {
        GrContextForegroundSet(pContext, ClrGreen);
        GrStringDraw(pContext, "RUN", -1, TELA_LARGURA - 22, 2, true);
    } else {
        GrContextForegroundSet(pContext, ClrRed);
        GrStringDraw(pContext, "STOP", -1, TELA_LARGURA - 28, 2, true);
    }

    // Rodapé: Escalas
    GrContextForegroundSet(pContext, ClrWhite);
    GrStringDraw(pContext, texto_vdiv, -1, 2, TELA_ALTURA - 10, true);
    GrStringDraw(pContext, texto_tdiv, -1, TELA_LARGURA - 55, TELA_ALTURA - 10, true);

    // Desenhar onda
    GrContextForegroundSet(pContext, ClrYellow); 

    if (num_pontos > TELA_LARGURA) {
        num_pontos = TELA_LARGURA;
    }

    int16_t x_anterior = 0;
    int16_t y_anterior = 0;
	//TODO: V_DIV
	// converte valor adc para tela
    for (uint16_t x = 0; x < num_pontos; x++) {
       
        uint32_t valor_adc = buffer[x];
        int16_t y_pixel = 127 - ((valor_adc * 127) / 4095);

        if (y_pixel < 0) y_pixel = 0;
        if (y_pixel > 127) y_pixel = 127;

        if (x > 0) {
            GrLineDraw(pContext, x_anterior, y_anterior, x, y_pixel);
        }

        x_anterior = x;
        y_anterior = y_pixel;
    }
}
