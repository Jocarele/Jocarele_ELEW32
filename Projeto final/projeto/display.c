#include "display.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"


// Define o tamanho da tela do BoosterPack
#define TELA_LARGURA 128
#define TELA_ALTURA  128

// Criação real do contexto da tela
tContext grContext;

// Criação real das variáveis globais
volatile EstadoTela estado_atual = TELA_PRINCIPAL;

int menu_selecionado = 0; 
int indice_taxa = 1;      
int indice_vdiv = 1;      
int indice_hdiv = 1;      
int indice_nivel_trig = 1;
int indice_borda_trig = 0;

const int valores_taxa[] = {1, 5, 10};
const int valores_vdiv[] = {5, 10, 20}; // 0.5, 1.0, 2.0
const int valores_hdiv[] = {1, 5, 10};
const int valores_nivel[] = {10, 15, 20}; // 1.0, 1.5, 2.0

const char* titulos_menu[] = {"Taxa (kHz)", "Escala V (V)", "Escala H (ms)", "Nivel (V)", "Borda"};
const char* str_borda_trig[] = {"Subida", "Descida"};



// ==========================================================
// FUNÇÕES DE DESENHO
// ==========================================================
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

    // ==========================================================
    // 1. LIMPAR A TELA (Fundo Preto)
    // ==========================================================
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &rectTela);

    // ==========================================================
    // 2. DESENHAR A GRADE (Graticule)
    // ==========================================================
    // Vamos fazer 8 divisões na tela (128 / 8 = 16 pixels por quadrado)
    GrContextForegroundSet(pContext, ClrDarkSlateGray); // Cor cinza escuro para não ofuscar a onda
		
    for (int i = 16; i < TELA_LARGURA; i += 16) {
        // Linhas verticais
        GrLineDraw(pContext, i, 0, i, TELA_ALTURA);
        // Linhas horizontais
        GrLineDraw(pContext, 0, i, TELA_LARGURA, i);
    }

    // ==========================================================
    // 3. DESENHAR OS TEXTOS DA INTERFACE (OSD)
    // ==========================================================
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextFontSet(pContext, g_psFontFixed6x8); // Fonte pequena padrão da GrLib

    // Canto superior esquerdo: Canal
    GrStringDraw(pContext, "CH1", -1, 2, 2, true);

    // Canto superior direito: Estado (RUN / STOP)
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

    // ==========================================================
    // 4. DESENHAR A FORMA DE ONDA
    // ==========================================================
    GrContextForegroundSet(pContext, ClrYellow); // A cor clássica do CH1

    // Previne desenhar mais pontos do que a tela suporta
    if (num_pontos > TELA_LARGURA) {
        num_pontos = TELA_LARGURA;
    }

    int16_t x_anterior = 0;
    int16_t y_anterior = 0;
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
