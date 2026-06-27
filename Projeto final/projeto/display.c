#include "display.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"


#define TELA_LARGURA 128
#define TELA_ALTURA  128
#define ADC_REF_MV        3300
#define ADC_CENTER_MV     1650
#define PIXELS_POR_DIV    16
#define TELA_CENTRO_Y     64



Onda_conf onda = {
    .taxa_khz = 1,
    .v_div = 1000,
    .h_div = 500,
    .modo_single = 0,
    .nivel = 0,
		.borda_trigger=0
	
};

tContext grContext;


volatile EstadoTela estado_atual = TELA_PRINCIPAL;

int menu_selecionado = 0; 
int indice_taxa = 1;      
int indice_vdiv = 1;      
int indice_hdiv = 1;      
int indice_nivel_trig = 1;
int indice_borda_trig = 0;
int indice_modo_single = 0;

const int valores_taxa[] = {1, 5, 10, 20}; // kHz
const int valores_vdiv[] = {500, 1000, 2000};
const int valores_hdiv[] = {500, 1000, 5000, 10000}; // us/div 
const int valores_nivel[] = {0, 500,1000,2000}; 

const char* titulos_menu[] = {
    "Taxa (kHz)",
    "Escala mV",
    "Escala H(us)",
    "Nivel Trig",
    "Borda Trig",
    "Modo"
};
const char* str_borda_trig[] = {"Subida", "Descida"};
const char* str_modo_single[] = {"continuo", "single-shot"};



/**
 * @brief Inicializa o display LCD e o contexto gráfico da biblioteca GrLib.
 *
 * Configura o display Crystalfontz 128x128, define a orientação da tela
 * e inicializa o contexto gráfico global utilizado pelas funções de desenho.
 *
 * @param None.
 * @return None.
 */
void DisplaySetup(void) {
    Crystalfontz128x128_Init();
    Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);
    GrContextInit(&grContext, &g_sCrystalfontz128x128);
    GrContextFontSet(&grContext, &g_sFontFixed6x8);
}

/**
 * @brief Converte um valor inteiro e sua unidade para texto.
 *
 * A função monta manualmente uma string no formato "valor unidade",
 * evitando o uso de sprintf para reduzir custo de processamento e uso
 * de memória em sistema embarcado.
 *
 * @param buf Ponteiro para o vetor de caracteres onde o texto será armazenado.
 * @param val Valor inteiro a ser convertido.
 * @param unidade Texto da unidade a ser acrescentada ao valor.
 *
 * @return None.
 */
void FormatarValor(char* buf, int val, const char* unidade) {
 
    char temp[12];
    int i = 0;
    int j = 0;

    if (val == 0)
    {
        buf[j++] = '0';
        buf[j] = '\0';
				if (*unidade != '\0'){
							buf[j++] = ' ';
							while(*unidade) buf[j++] = *unidade++;
							buf[j] = '\0';
				}
        return;
    }

    if (val < 0)
    {
        buf[j++] = '-';
        val = -val;
    }

    while (val > 0)
    {
        temp[i++] = (val % 10) + '0';
        val /= 10;
    }

    while (i > 0)
    {
        buf[j++] = temp[--i];
    }

		buf[j++] = ' ';
    while(*unidade) buf[j++] = *unidade++;
		buf[j] = '\0';

}

/**
 * @brief Desenha o menu principal de configurações na tela.
 *
 * Limpa o display, desenha o título do menu e lista as opções disponíveis.
 * A opção atualmente selecionada é destacada visualmente.
 *
 * @param None.
 * @return None.
 */
void DesenharMenuPrincipal(void) {
    tRectangle rectFundo = {0, 0, 127, 127};
    GrContextForegroundSet(&grContext, ClrBlack);
    GrRectFill(&grContext, &rectFundo);

    // Cabeçalho
    GrContextForegroundSet(&grContext, ClrWhite);
    GrContextFontSet(&grContext, g_psFontFixed6x8);
    GrStringDrawCentered(&grContext, "CONFIGURACOES", -1, 64, 8, false);
    GrLineDraw(&grContext, 0, 16, 127, 16);

    for(int i = 0; i < 6; i++) {
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
/**
 * @brief Desenha uma tela de submenu para ajuste de parâmetro.
 *
 * Mostra o título do parâmetro, o valor atual formatado com sua unidade
 * e indicações visuais para alteração com o joystick.
 *
 * @param titulo Texto exibido no topo do submenu.
 * @param valor Valor atual do parâmetro selecionado.
 * @param unidade Unidade associada ao parâmetro.
 *
 * @return None.
 */
void DesenharSubMenu(const char* titulo, int valor, const char* unidade) {
    char texto_valor[20];
    tRectangle rectFundo = {0, 0, 127, 127};

    GrContextForegroundSet(&grContext, ClrBlack);
    GrRectFill(&grContext, &rectFundo);

    // Título
    GrContextForegroundSet(&grContext, ClrWhite);
    GrStringDrawCentered(&grContext, titulo, -1, 64, 10, false);
    GrLineDraw(&grContext, 0, 20, 127, 20);

    FormatarValor(texto_valor, valor, unidade);

    // Valor Central
    GrStringDrawCentered(&grContext, "<", -1, 20, 64, false);
    GrStringDrawCentered(&grContext, texto_valor, -1, 64, 64, false);
    GrStringDrawCentered(&grContext, ">", -1, 108, 64, false);

    // Rodapé
    GrLineDraw(&grContext, 0, 105, 127, 105);
    GrStringDraw(&grContext, "Voltar", -1, 5, 115, false);
    GrStringDraw(&grContext, "Sel", -1, 95, 115, false); 
    GrFlush(&grContext);
}
/**
 * @brief Converte uma amostra ADC em coordenada vertical da tela.
 *
 * A conversão considera referência de 3,3 V, centro vertical em 1,65 V
 * e a escala vertical configurada em mV/div. Valores fora da área útil
 * são limitados entre 0 e 127 pixels.
 *
 * @param valor_adc Valor lido do ADC, esperado entre 0 e 4095.
 *
 * @return Coordenada Y correspondente no display.
 */
static int16_t ADCParaYPixel(uint32_t valor_adc)
{
    int32_t tensao_mv;
    int32_t tensao_relativa_mv;
    int32_t y;
	
    tensao_mv = ((int32_t)valor_adc * ADC_REF_MV) / 4095;

    tensao_relativa_mv = tensao_mv - ADC_CENTER_MV;

    y = TELA_CENTRO_Y - ((tensao_relativa_mv * PIXELS_POR_DIV) / onda.v_div);

    if (y < 0)
        y = 0;

    if (y > 127)
        y = 127;

    return (int16_t)y;
}
/**
 * @brief Desenha a tela principal do osciloscópio no modo ponto a ponto.
 *
 * A função limpa a tela, desenha a grade, os textos de interface
 * e a forma de onda a partir de um vetor de amostras ADC. Esse modo
 * é usado quando a janela temporal possui até 128 amostras.
 *
 * @param pContext Ponteiro para o contexto gráfico da GrLib.
 * @param buffer Vetor com as amostras ADC a serem desenhadas.
 * @param num_pontos Quantidade de pontos válidos no vetor.
 * @param texto_vdiv Texto da escala vertical exibida no rodapé.
 * @param texto_tdiv Texto da escala horizontal exibida no rodapé.
 * @param is_running Indica se o osciloscópio está em execução ou parado.
 *
 * @return None.
 */
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
    for (int i = 16; i < TELA_LARGURA; i += 16) {
        GrLineDraw(pContext, 0, i, TELA_LARGURA, i);
    }
	for (int i = 16; i < TELA_ALTURA; i += 16) {
        GrLineDraw(pContext, i, 0, i, TELA_ALTURA);
    }

    // texto interface
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextFontSet(pContext, g_psFontFixed6x8);

    // Canto superior esquerdo: Canal
    GrStringDraw(pContext, "CH1", -1, 2, 2, true);

    // run/stop 
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

    for (uint16_t x = 0; x < num_pontos; x++) {
       
				uint32_t valor_adc = buffer[x];

				int16_t y_pixel = ADCParaYPixel(valor_adc);

        if (x > 0) {
            GrLineDraw(pContext, x_anterior, y_anterior, x, y_pixel);
        }

        x_anterior = x;
        y_anterior = y_pixel;
    }
}


/**
 * @brief Desenha a tela principal do osciloscópio usando compactação por buckets.
 *
 * Esse modo é utilizado quando a janela temporal possui mais amostras
 * do que pixels horizontais disponíveis. Para cada coluna da tela,
 * são usados o primeiro valor, o último valor, o mínimo, o máximo
 * e a ordem temporal dos extremos, preservando melhor a amplitude
 * e a variação do sinal.
 *
 * @param pContext Ponteiro para o contexto gráfico da GrLib.
 * @param buckets Vetor de buckets, um para cada coluna da tela.
 * @param num_pontos Quantidade de buckets válidos.
 * @param texto_vdiv Texto da escala vertical exibida no rodapé.
 * @param texto_tdiv Texto da escala horizontal exibida no rodapé.
 * @param is_running Indica se o osciloscópio está em execução ou parado.
 *
 * @return None.
 */
void DesenharOsciloscopio2(tContext *pContext,
                                 PixelBucket *buckets,
                                 uint16_t num_pontos,
                                 const char* texto_vdiv,
                                 const char* texto_tdiv,
                                 bool is_running)
{
    tRectangle rectTela;

    int16_t y_primeiro;
    int16_t y_ultimo;
    int16_t y_min;
    int16_t y_max;

    int16_t y_ant;
    bool tem_anterior = false;

    rectTela.i16XMin = 0;
    rectTela.i16YMin = 0;
    rectTela.i16XMax = TELA_LARGURA - 1;
    rectTela.i16YMax = TELA_ALTURA - 1;


    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &rectTela);
    GrContextForegroundSet(pContext, ClrDarkSlateGray);

    for (int i = 16; i < TELA_LARGURA; i += 16)
    {
        GrLineDraw(pContext, 0, i, TELA_LARGURA, i);
    }

    for (int i = 16; i < TELA_ALTURA; i += 16)
    {
        GrLineDraw(pContext, i, 0, i, TELA_ALTURA);
    }

    GrContextForegroundSet(pContext, ClrWhite);
    GrContextFontSet(pContext, g_psFontFixed6x8);
    GrStringDraw(pContext, "CH1", -1, 2, 2, true);
    if (is_running)
    {
        GrContextForegroundSet(pContext, ClrGreen);
        GrStringDraw(pContext, "RUN", -1, TELA_LARGURA - 22, 2, true);
    }
    else
    {
        GrContextForegroundSet(pContext, ClrRed);
        GrStringDraw(pContext, "STOP", -1, TELA_LARGURA - 28, 2, true);
    }
    GrContextForegroundSet(pContext, ClrWhite);
    GrStringDraw(pContext, texto_vdiv, -1, 2, TELA_ALTURA - 10, true);
    GrStringDraw(pContext, texto_tdiv, -1, TELA_LARGURA - 55, TELA_ALTURA - 10, true);

    if (num_pontos > TELA_PONTOS)
        num_pontos = TELA_PONTOS;
    GrContextForegroundSet(pContext, ClrYellow);

    for (uint16_t x = 0; x < num_pontos; x++)
    {
        y_primeiro = ADCParaYPixel(buckets[x].primeiro);
        y_ultimo   = ADCParaYPixel(buckets[x].ultimo);
        y_min      = ADCParaYPixel(buckets[x].minimo);
        y_max      = ADCParaYPixel(buckets[x].maximo);

        if (tem_anterior)
        {
            GrLineDraw(pContext, x - 1, y_ant, x, y_primeiro);
        }
        if (buckets[x].qtd <= 1)
        {
            GrLineDraw(pContext, x, y_primeiro, x, y_ultimo);
        }
        else
        {
            if (buckets[x].pos_min < buckets[x].pos_max)
            {
                GrLineDraw(pContext, x, y_primeiro, x, y_min);
                GrLineDraw(pContext, x, y_min, x, y_max);
                GrLineDraw(pContext, x, y_max, x, y_ultimo);
            }
            else if (buckets[x].pos_max < buckets[x].pos_min)
            {
                GrLineDraw(pContext, x, y_primeiro, x, y_max);
                GrLineDraw(pContext, x, y_max, x, y_min);
                GrLineDraw(pContext, x, y_min, x, y_ultimo);
            }
            else
            {
                GrLineDraw(pContext, x, y_primeiro, x, y_ultimo);
            }
        }

        y_ant = y_ultimo;
        tem_anterior = true;
    }
}