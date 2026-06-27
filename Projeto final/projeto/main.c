/*__________________________________________________________________________________
|       Disciplina de Sistemas Embarcados - 2026-1
|       Prof. Douglas Renaux
| __________________________________________________________________________________
|
|       PROJETO FINAL
| __________________________________________________________________________________
*/

/**
 * @file     main.c
 * @author   Bruno Ribeiro Basilio
						 João Lucas Marques Camilo
 * @brief   Projeto final da disciplina de sistemas embarcados
 *            
 *            
 *
 ******************************************************************************/

/*------------------------------------------------------------------------------
 *
 *      File includes
 *
 *------------------------------------------------------------------------------*/
#define OSC_USAR_SIMULADOR 1
#include <stdint.h>
#include <stdbool.h>
// Bibliotecas Base da TivaWare
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"

// Biblioteca ThreadX
#include "tx_api.h"

#include "grlib/grlib.h"
// Bibliotecas de Periféricos (DriverLib)
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/adc.h"      
#include "driverlib/timer.h"
#include "Crystalfontz128x128_ST7735.h"
#include "HAL_EK_TM4C1294XL_Crystalfontz128x128_ST7735.h"
#include "display.h"
#include "hardware.h"

#if OSC_USAR_SIMULADOR
#include "simula.h"
#endif

/*------------------------------------------------------------------------------
 *
 *      Typedefs and constants
 *
 *------------------------------------------------------------------------------*/
#define DEMO_STACK_SIZE         2048
#define DEMO_BYTE_POOL_SIZE     9120
#define PART_TM4C1294NCPDT
#define TARGET_IS_TM4C129_RA1
#define BUFFER_SIZE 2048
#define TRIGGER_POS_PERCENT 25
#define TELA_PONTOS 128

typedef struct {
    uint32_t data[BUFFER_SIZE];
    volatile uint16_t head; 
    volatile uint16_t tail; 
    volatile uint16_t count; 
} CircularBuffer;
/*------------------------------------------------------------------------------
 *
 *      Global vars
 *
 *------------------------------------------------------------------------------*/
TX_THREAD               thread_0;
TX_THREAD               thread_1;
TX_MUTEX                display_mutex;
TX_BYTE_POOL            byte_pool_0;
TX_EVENT_FLAGS_GROUP    flag_0; 
UCHAR                   memory_area[DEMO_BYTE_POOL_SIZE];
uint32_t ui32SysClock;
CircularBuffer osc_buffer = { .head = 0, .tail = 0, .count = 0 };

static uint32_t amostras_tela_global[TELA_PONTOS];
static PixelBucket buckets_tela_global[TELA_PONTOS];

/*------------------------------------------------------------------------------
 *
 *      File scope vars
 *
 *------------------------------------------------------------------------------*/

static volatile uint8_t single_congelado = 0;


/*------------------------------------------------------------------------------
 *
 *      Function prototypes
 *
 *------------------------------------------------------------------------------*/
void tx_application_define(void * first_unused_memory);

void    thread_0_entry(ULONG thread_input);
void   	thread_1_entry(ULONG thread_input);
uint32_t AmostrasJanela(void);
uint16_t MontarAmostrasTela(uint32_t *amostras_tela);
void LimparOscBuffer(void);
void AtualizarTaxaAmostragem(uint32_t freq_hz);
void OscBufferPush(uint32_t valor);
uint32_t NivelTriggerADC(void);
bool DetectouTrigger(uint32_t anterior, uint32_t atual);
int32_t ProcurarTrigger(uint32_t pre_trigger,uint32_t post_trigger,uint32_t disponiveis);
void RearmarSingleShot(void);
bool SingleShotCongelado(void);
uint16_t MontarAmostrasTela2(PixelBucket *buckets);




/*------------------------------------------------------------------------------
 *
 *      Functions
 *
 *------------------------------------------------------------------------------*/

/**
 * @brief Monta os buckets usados na visualização compactada da forma de onda.
 *
 * Para cada coluna horizontal da tela, a função agrupa as amostras
 * correspondentes e armazena o primeiro valor, último valor, mínimo,
 * máximo e a posição relativa dos extremos. Esse método é usado quando
 * a janela temporal contém mais amostras do que pixels horizontais.
 *
 * A função também aplica o alinhamento por trigger e respeita o modo
 * single-shot, congelando a tela quando necessário.
 *
 * @param buckets Vetor onde os buckets serão armazenados.
 *
 * @return Quantidade de buckets montados para desenho.
 */
uint16_t MontarAmostrasTela2(PixelBucket *buckets)
{
    uint32_t amostras_janela;
    uint32_t disponiveis;
    uint32_t usadas;

    uint32_t pre_trigger;
    uint32_t post_trigger;

    uint32_t inicio_tela;
    uint32_t idx_buffer;

    uint32_t ini_rel;
    uint32_t fim_rel;

    uint32_t valor;
    uint32_t rel_pos;

    int32_t pos_trigger;
    bool achou_trigger = false;

    if (onda.modo_single && single_congelado)
    {
        return 0;
    }

#if OSC_USAR_SIMULADOR
    MAP_IntDisable(INT_TIMER0A_TM4C129);
#else
    MAP_IntDisable(INT_ADC1SS0_TM4C129);
#endif

    amostras_janela = AmostrasJanela();
    disponiveis = osc_buffer.count;

    if (disponiveis < 2)
    {
#if OSC_USAR_SIMULADOR
        MAP_IntEnable(INT_TIMER0A_TM4C129);
#else
        MAP_IntEnable(INT_ADC1SS0_TM4C129);
#endif
        return 0;
    }

    if (amostras_janela > disponiveis)
        usadas = disponiveis;
    else
        usadas = amostras_janela;

    pre_trigger = (usadas * TRIGGER_POS_PERCENT) / 100;

    if (pre_trigger < 1)
        pre_trigger = 1;

    post_trigger = usadas - pre_trigger;

    if (post_trigger < 1)
        post_trigger = 1;

    pos_trigger = ProcurarTrigger(pre_trigger,
                                          post_trigger,
                                          disponiveis);

    if (pos_trigger >= 0)
    {
        inicio_tela = (osc_buffer.tail +
                       (uint32_t)pos_trigger +
                       BUFFER_SIZE -
                       pre_trigger) % BUFFER_SIZE;

        achou_trigger = true;
    }
    else
    {
        inicio_tela = (osc_buffer.head + BUFFER_SIZE - usadas) % BUFFER_SIZE;
        achou_trigger = false;
    }
	
    for (uint16_t x = 0; x < TELA_PONTOS; x++)
    {
        ini_rel = ((uint32_t)x * usadas) / TELA_PONTOS;
        fim_rel = ((uint32_t)(x + 1) * usadas) / TELA_PONTOS;

        if (fim_rel <= ini_rel)
            fim_rel = ini_rel + 1;

        if (fim_rel > usadas)
            fim_rel = usadas;

        idx_buffer = (inicio_tela + ini_rel) % BUFFER_SIZE;
        valor = osc_buffer.data[idx_buffer];

        buckets[x].primeiro = valor;
        buckets[x].ultimo = valor;
        buckets[x].minimo = valor;
        buckets[x].maximo = valor;
        buckets[x].pos_min = 0;
        buckets[x].pos_max = 0;
        buckets[x].qtd = fim_rel - ini_rel;

        for (uint32_t k = ini_rel; k < fim_rel; k++)
        {
            rel_pos = k - ini_rel;
            idx_buffer = (inicio_tela + k) % BUFFER_SIZE;
            valor = osc_buffer.data[idx_buffer];

            if (valor < buckets[x].minimo)
            {
                buckets[x].minimo = valor;
                buckets[x].pos_min = rel_pos;
            }

            if (valor > buckets[x].maximo)
            {
                buckets[x].maximo = valor;
                buckets[x].pos_max = rel_pos;
            }

            buckets[x].ultimo = valor;
        }
    }

    if (onda.modo_single && achou_trigger)
    {
        single_congelado = 1;
    }

#if OSC_USAR_SIMULADOR
    MAP_IntEnable(INT_TIMER0A_TM4C129);
#else
    MAP_IntEnable(INT_ADC1SS0_TM4C129);
#endif

    return TELA_PONTOS;
}
/**
 * @brief Converte o nível de trigger configurado em mV para valor ADC.
 *
 * O nível de trigger armazenado em onda.nivel é limitado à faixa de
 * 0 a 3300 mV e convertido para a escala de 12 bits do ADC.
 *
 * @param None.
 *
 * @return Valor correspondente do nível de trigger na escala ADC.
 */
uint32_t NivelTriggerADC(void)
{
    uint32_t nivel_mv = onda.nivel;

    if (nivel_mv > 3300)
        nivel_mv = 3300;

    return (nivel_mv * 4095) / 3300;
}
/**
 * @brief Verifica se houve cruzamento do nível de trigger.
 *
 * Compara duas amostras consecutivas e identifica cruzamento do nível
 * configurado, considerando a borda selecionada: subida ou descida.
 *
 * @param anterior Amostra anterior do sinal.
 * @param atual Amostra atual do sinal.
 *
 * @return true se o trigger foi detectado, false caso contrário.
 */
bool DetectouTrigger(uint32_t anterior, uint32_t atual)
{
    uint32_t nivel_adc = NivelTriggerADC();

    if (onda.borda_trigger == 0)
    {
        if (anterior < nivel_adc && atual >= nivel_adc)
            return true;
    }
    else
    {
        if (anterior > nivel_adc && atual <= nivel_adc)
            return true;
    }

    return false;
}
/**
 * @brief Procura uma posição de trigger válida no buffer circular.
 *
 * A busca é feita de trás para frente para encontrar o trigger mais
 * recente que ainda possua amostras suficientes antes e depois dele.
 * Isso permite alinhar a forma de onda na tela com uma porcentagem
 * de pré-trigger.
 *
 * @param pre_trigger Quantidade de amostras desejadas antes do trigger.
 * @param post_trigger Quantidade de amostras desejadas depois do trigger.
 * @param disponiveis Quantidade de amostras disponíveis no buffer circular.
 *
 * @return Índice relativo do trigger dentro do buffer ou -1 se não encontrado.
 */
int32_t ProcurarTrigger(uint32_t pre_trigger,
                                uint32_t post_trigger,
                                uint32_t disponiveis)
{
    int32_t i;

    if (disponiveis < 2)
        return -1;

    for (i = (int32_t)(disponiveis - post_trigger);
         i >= (int32_t)pre_trigger;
         i--)
    {
        uint32_t idx_ant;
        uint32_t idx_atual;
        uint32_t anterior;
        uint32_t atual;

        idx_ant   = (osc_buffer.tail + (uint32_t)i - 1) % BUFFER_SIZE;
        idx_atual = (osc_buffer.tail + (uint32_t)i) % BUFFER_SIZE;

        anterior = osc_buffer.data[idx_ant];
        atual    = osc_buffer.data[idx_atual];

        if (DetectouTrigger(anterior, atual))
        {
            return i;
        }
    }

    return -1;
}
/**
 * @brief Rearma o modo single-shot.
 *
 * Descongela a aquisição em modo single-shot, permitindo que um novo
 * evento de trigger seja capturado. Durante a alteração da flag de
 * congelamento, a interrupção de aquisição é temporariamente desabilitada.
 *
 * @param None.
 * @return None.
 */
void RearmarSingleShot(void)
{
	#if OSC_USAR_SIMULADOR
			MAP_IntDisable(INT_TIMER0A_TM4C129);
	#else
			MAP_IntDisable(INT_ADC1SS0_TM4C129);
	#endif

			single_congelado = 0;

	#if OSC_USAR_SIMULADOR
			MAP_IntEnable(INT_TIMER0A_TM4C129);
	#else
			MAP_IntEnable(INT_ADC1SS0_TM4C129);
	#endif
}
/**
 * @brief Informa se o modo single-shot está congelado.
 *
 * Essa função é usada pela thread de visualização para decidir se deve
 * exibir o estado RUN ou STOP na tela.
 *
 * @param None.
 *
 * @return true se o single-shot estiver congelado, false caso contrário.
 */
bool SingleShotCongelado(void)
{
    return single_congelado ? true : false;
}

/**
 * @brief Insere uma nova amostra no buffer circular do osciloscópio.
 *
 * A função limita o valor à faixa válida do ADC e armazena a amostra
 * na posição atual de escrita. Quando o buffer está cheio, a amostra
 * mais antiga é descartada automaticamente.
 *
 * @param valor Valor ADC a ser inserido no buffer.
 *
 * @return None.
 */
void OscBufferPush(uint32_t valor)
{
    if (valor > 4095)
        valor = 4095;

    osc_buffer.data[osc_buffer.head] = valor;

    osc_buffer.head = (osc_buffer.head + 1) % BUFFER_SIZE;

    if (osc_buffer.count < BUFFER_SIZE)
    {
        osc_buffer.count++;
    }
    else
    {
        osc_buffer.tail = (osc_buffer.tail + 1) % BUFFER_SIZE;
    }
}

/**
 * @brief Limpa o buffer circular de amostras.
 *
 * Zera os índices de leitura, escrita e contagem de amostras. A interrupção
 * de aquisição é temporariamente desabilitada para evitar escrita no buffer
 * durante a limpeza.
 *
 * @param None.
 * @return None.
 */
void LimparOscBuffer(void)
{
#if OSC_USAR_SIMULADOR
    MAP_IntDisable(INT_TIMER0A_TM4C129);
#else
    MAP_IntDisable(INT_ADC1SS0_TM4C129);
#endif

    osc_buffer.count = 0;
    osc_buffer.head = 0;
    osc_buffer.tail = 0;

#if OSC_USAR_SIMULADOR
    MAP_IntEnable(INT_TIMER0A_TM4C129);
#else
    MAP_IntEnable(INT_ADC1SS0_TM4C129);
#endif
}

/**
 * @brief Atualiza a taxa de amostragem do osciloscópio.
 *
 * Limpa o buffer circular e reconfigura a frequência de aquisição.
 * No modo simulador, atualiza o Timer0 usado para gerar amostras falsas.
 * No modo real, atualiza o Timer0 usado como gatilho do ADC1.
 *
 * @param freq_hz Nova taxa de amostragem em hertz.
 *
 * @return None.
 */
void AtualizarTaxaAmostragem(uint32_t freq_hz)
{
    

    if (freq_hz == 0)
        return;

    LimparOscBuffer();

#if OSC_USAR_SIMULADOR

    SimuladorADC_AtualizarTaxa(ui32SysClock, freq_hz);

#else
		uint32_t carga;
    carga = (ui32SysClock / freq_hz) - 1;

    MAP_TimerDisable(TIMER0_BASE, TIMER_A);
    MAP_IntDisable(INT_ADC1SS0_TM4C129);

    MAP_TimerLoadSet(TIMER0_BASE, TIMER_A, carga);

    MAP_ADCIntClear(ADC1_BASE, 0);
    MAP_TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    MAP_IntEnable(INT_ADC1SS0_TM4C129);
    MAP_TimerEnable(TIMER0_BASE, TIMER_A);

#endif
}

/**
 * @brief Calcula a quantidade de amostras na janela horizontal da tela.
 *
 * Usa a taxa de amostragem configurada e a escala horizontal em us/div
 * para calcular quantas amostras representam as 8 divisões horizontais
 * do display. O valor é limitado ao tamanho do buffer circular.
 *
 * @param None.
 *
 * @return Quantidade de amostras que compõem a janela temporal atual.
 */
uint32_t AmostrasJanela(void)
{
    uint32_t taxa_hz = onda.taxa_khz * 1000;
    uint32_t tempo_total_us = onda.h_div * 8;

    uint32_t amostras;

    amostras = ((uint64_t)taxa_hz * tempo_total_us) / 1000000;

    if (amostras < 2)
        amostras = 2;

    if (amostras > BUFFER_SIZE)
        amostras = BUFFER_SIZE;

    return amostras;
}
/**
 * @brief Monta o vetor de amostras para desenho ponto a ponto.
 *
 * A função seleciona a janela temporal atual do buffer circular,
 * alinha a visualização pelo trigger quando possível e gera 128 pontos
 * para a tela usando interpolação linear. Esse modo é usado quando
 * a quantidade de amostras da janela é menor ou igual à largura do display.
 *
 * @param amostras_tela Vetor onde os pontos interpolados serão armazenados.
 *
 * @return Quantidade de pontos montados para desenho.
 */
uint16_t MontarAmostrasTela(uint32_t *amostras_tela)
{
    uint32_t amostras_janela;
    uint32_t disponiveis;
    uint32_t usadas;

    uint32_t pre_trigger;
    uint32_t post_trigger;

    uint32_t inicio_tela;

    int32_t pos_trigger;
    bool achou_trigger = false;

    if (onda.modo_single && single_congelado)
    {
        return 0;
    }

		#if OSC_USAR_SIMULADOR
				MAP_IntDisable(INT_TIMER0A_TM4C129);
		#else
				MAP_IntDisable(INT_ADC1SS0_TM4C129);
		#endif

    amostras_janela = AmostrasJanela();
    disponiveis = osc_buffer.count;

    if (disponiveis < 2)
    {
		#if OSC_USAR_SIMULADOR
						MAP_IntEnable(INT_TIMER0A_TM4C129);
		#else
						MAP_IntEnable(INT_ADC1SS0_TM4C129);
		#endif
        return 0;
    }

    if (amostras_janela > disponiveis)
        usadas = disponiveis;
    else
        usadas = amostras_janela;

    pre_trigger = (usadas * TRIGGER_POS_PERCENT) / 100;

    if (pre_trigger < 1)
        pre_trigger = 1;

    post_trigger = usadas - pre_trigger;

    if (post_trigger < 1)
        post_trigger = 1;

    pos_trigger = ProcurarTrigger(pre_trigger,
                                          post_trigger,
                                          disponiveis);

    if (pos_trigger >= 0)
    {
        inicio_tela = (osc_buffer.tail +
                       (uint32_t)pos_trigger +
                       BUFFER_SIZE -
                       pre_trigger) % BUFFER_SIZE;

        achou_trigger = true;
    }
    else
    {
        inicio_tela = (osc_buffer.head + BUFFER_SIZE - usadas) % BUFFER_SIZE;
        achou_trigger = false;
    }

    uint32_t den = TELA_PONTOS - 1;

		for (uint16_t x = 0; x < TELA_PONTOS; x++)
		{
				uint32_t pos_num;
				uint32_t idx0_rel;
				uint32_t idx1_rel;
				uint32_t frac;
				uint32_t idx0_buffer;
				uint32_t idx1_buffer;
				uint32_t v0;
				uint32_t v1;
				uint32_t valor;

				pos_num = ((uint32_t)x * (usadas - 1));

				idx0_rel = pos_num / den;
				frac = pos_num % den;

				if (idx0_rel + 1 < usadas)
						idx1_rel = idx0_rel + 1;
				else
						idx1_rel = idx0_rel;

				idx0_buffer = (inicio_tela + idx0_rel) % BUFFER_SIZE;
				idx1_buffer = (inicio_tela + idx1_rel) % BUFFER_SIZE;

				v0 = osc_buffer.data[idx0_buffer];
				v1 = osc_buffer.data[idx1_buffer];

				valor = ((v0 * (den - frac)) + (v1 * frac)) / den;

				amostras_tela[x] = valor;
		}

    if (onda.modo_single && achou_trigger)
    {
        single_congelado = 1;
    }

#if OSC_USAR_SIMULADOR
    MAP_IntEnable(INT_TIMER0A_TM4C129);
#else
    MAP_IntEnable(INT_ADC1SS0_TM4C129);
#endif

    return TELA_PONTOS;
}


/**
 * @brief Função principal do programa.
 *
 * Configura o clock principal do microcontrolador, inicializa o display
 * e inicia o kernel do ThreadX. A partir desse ponto, a execução passa
 * a ser controlada pelas threads criadas em tx_application_define().
 *
 * @param None.
 *
 * @return Código de retorno da aplicação.
 */

int main()
{

    ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
                                       SYSCTL_OSC_MAIN |
                                       SYSCTL_USE_PLL |
                                       SYSCTL_CFG_VCO_240), 120000000);

    DisplaySetup();
		

    tx_kernel_enter();

    return 0;
}

/**
 * @brief Define e cria os objetos principais do ThreadX.
 *
 * Cria o byte pool, mutex do display, grupo de flags de eventos e as
 * threads principais do sistema. A thread_0 é responsável pela visualização
 * do osciloscópio e a thread_1 pelo menu e leitura do joystick.
 *
 * @param first_unused_memory Ponteiro fornecido pelo ThreadX para memória livre.
 *
 * @return None.
 */
void tx_application_define(void *first_unused_memory)
{
    (void)first_unused_memory;

    CHAR *pointer = TX_NULL;
    UINT status;

    /*
     * 1. Cria o byte pool primeiro.
     */
    status = tx_byte_pool_create(&byte_pool_0,"byte pool 0",memory_area,DEMO_BYTE_POOL_SIZE);

    if (status != TX_SUCCESS)
        while(1) {}

    /*
     * 2. Cria objetos de sincronização.
     */
    status = tx_mutex_create(&display_mutex, "mutex 0",TX_NO_INHERIT);

    if (status != TX_SUCCESS)
        while(1) {}

    status = tx_event_flags_create(&flag_0,"flag evento");

    if (status != TX_SUCCESS)
        while(1) {}

    estado_atual = TELA_PRINCIPAL;

    /*
     * 3. Aloca stack e cria thread_0.
     */
    status = tx_byte_allocate(&byte_pool_0,(VOID **) &pointer,DEMO_STACK_SIZE,TX_NO_WAIT);
    if (status != TX_SUCCESS)
        while(1) {}

    status = tx_thread_create(&thread_0,"thread 0",thread_0_entry,0,pointer,DEMO_STACK_SIZE,9,9,TX_NO_TIME_SLICE,TX_AUTO_START);

    if (status != TX_SUCCESS)
        while(1) {}

    /*
     * 4. Aloca stack e cria thread_1.
     */
    pointer = TX_NULL;
    status = tx_byte_allocate(&byte_pool_0,(VOID **) &pointer,DEMO_STACK_SIZE,TX_NO_WAIT);

    if (status != TX_SUCCESS)
        while(1) {}

    status = tx_thread_create(&thread_1,"thread 1",thread_1_entry,1,pointer,DEMO_STACK_SIZE,10,10,TX_NO_TIME_SLICE,TX_AUTO_START);

    if (status != TX_SUCCESS)
        while(1) {}
}


/**
 * @brief Thread responsável pela visualização do osciloscópio.
 *
 * Inicializa o temporizador de atualização da tela, configura a aquisição
 * real ou o simulador e redesenha periodicamente a tela principal.
 * Dependendo da quantidade de amostras na janela, escolhe entre desenho
 * ponto a ponto ou desenho compactado por buckets.
 *
 * @param thread_input Parâmetro de entrada da thread, não utilizado.
 *
 * @return None.
 */
void thread_0_entry(ULONG thread_input)
{
    (void)thread_input;
    ULONG actual_flags;
    uint32_t amostras_janela;

	
    bool is_running = true;
		ConfigurarTimer1A(300);
		char buf1[20];
		char buf2[20];
		const char* str_vdiv = "mV/div";
    const char* str_tdiv = "us/div";

    #if OSC_USAR_SIMULADOR
			SimuladorADC_SetTipoOnda(SIM_ONDA_SENOIDAL);
			/* SimuladorADC_SetTipoOnda(SIM_ONDA_TRIANGULAR); */
			SimuladorADC_SetFrequenciaOnda(2000);
			SimuladorADC_SetAmplitudeOffset(1500, 2048);
			SimuladorADC_Configurar(ui32SysClock, onda.taxa_khz * 1000);
		#else
				ConfigurarOsciloscopioBackground();
		#endif
		tx_event_flags_set(&flag_0, 0x02, TX_OR);

    while(1)
    {
			tx_event_flags_get(&flag_0, 0x04, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
			uint16_t pontos_lidos = 0;
	
		
			if (estado_atual == TELA_PRINCIPAL)
			{
					FormatarValor(buf1, onda.v_div, str_vdiv);
					FormatarValor(buf2, onda.h_div, str_tdiv);

					amostras_janela = AmostrasJanela();

					if (amostras_janela <= TELA_PONTOS)
					{
							pontos_lidos = MontarAmostrasTela(amostras_tela_global);

							if (onda.modo_single && SingleShotCongelado())
									is_running = false;
							else
									is_running = true;

							if (pontos_lidos > 1)
							{
									tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);

									DesenharOsciloscopio(&grContext,
                     amostras_tela_global,
                     pontos_lidos,
                     buf1,
                     buf2,
                     is_running);

									GrFlush(&grContext);

									tx_mutex_put(&display_mutex);
							}
					}
					else
					{
							pontos_lidos = MontarAmostrasTela2(buckets_tela_global);

							if (onda.modo_single && SingleShotCongelado())
									is_running = false;
							else
									is_running = true;

							if (pontos_lidos > 1)
							{
									tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);

									DesenharOsciloscopio2(&grContext,
                            buckets_tela_global,
                            pontos_lidos,
                            buf1,
                            buf2,
                            is_running);

									GrFlush(&grContext);

									tx_mutex_put(&display_mutex);
							}
					}
			}
			else
			{
					LimparOscBuffer();
			}
    }
}
/**
 * @brief Thread responsável pelo menu e pela leitura do joystick.
 *
 * Configura o ADC do joystick, detecta cliques e movimentações,
 * navega entre o menu principal e os submenus e atualiza os parâmetros
 * globais do osciloscópio conforme a seleção do usuário.
 *
 * @param thread_input Parâmetro de entrada da thread, não utilizado.
 *
 * @return None.
 */
void thread_1_entry(ULONG thread_input)
{		
	ConfigurarADC0();
	
    (void)thread_input;
    ULONG actual_flags;
    uint32_t adc_joy[2];
		tx_event_flags_get(&flag_0, 0x02, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
    
    while(1)
    {
        // 1. MODO VISUALIZAÇÃO: 
        if (estado_atual == TELA_PRINCIPAL) {
            tx_event_flags_get(&flag_0, 0x01, TX_OR_CLEAR, &actual_flags, TX_WAIT_FOREVER);
            
            estado_atual = MENU_PRINCIPAL;
            menu_selecionado = 0;
					
						tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
            DesenharMenuPrincipal();
						tx_mutex_put(&display_mutex);
            tx_thread_sleep(50); 
        }

        // 2. MODO CONFIGURAÇÃO:
        while (estado_atual != TELA_PRINCIPAL) 
        {
            MAP_ADCProcessorTrigger(ADC0_BASE, 0);
            while(!MAP_ADCIntStatus(ADC0_BASE, 0, false)) {}
            MAP_ADCIntClear(ADC0_BASE, 0);
            MAP_ADCSequenceDataGet(ADC0_BASE, 0, adc_joy);
            
            uint32_t joy_x = adc_joy[0]; 
            uint32_t joy_y = adc_joy[1]; 
            
            bool clicou = (tx_event_flags_get(&flag_0, 0x01, TX_OR_CLEAR, &actual_flags, TX_NO_WAIT) == TX_SUCCESS);
            // NAVEGAÇÃO: MENU PRINCIPAL
            if (estado_atual == MENU_PRINCIPAL) {
								
                if (joy_y > 3000 && menu_selecionado > 0) { 
                    menu_selecionado--; 
					
										tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                    DesenharMenuPrincipal(); 
										tx_mutex_put(&display_mutex);
									
                }
                if (joy_y < 1000 && menu_selecionado < 5) { 
                    menu_selecionado++; 
										tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                    DesenharMenuPrincipal(); 
										tx_mutex_put(&display_mutex);
                }
				if (joy_x < 200) {
					estado_atual = TELA_PRINCIPAL;
					RearmarSingleShot();
					tx_thread_sleep(50);
				}
                
                if (clicou) {
                    if (menu_selecionado == 0) estado_atual = MENU_TAXA;
                    if (menu_selecionado == 1) estado_atual = MENU_VDIV;
                    if (menu_selecionado == 2) estado_atual = MENU_HDIV;
                    if (menu_selecionado == 3) estado_atual = MENU_NIVEL_TRIG;
                    if (menu_selecionado == 4) estado_atual = MENU_BORDA_TRIG;
										if (menu_selecionado == 5) estado_atual = MENU_MODO_SINGLE;

                }
            }
            // NAVEGAÇÃO: SUBMENUS 
            else {
                int* ptr_indice = NULL;
                int max_indice = 2; 
                const char* titulo = "";
                int valor = 0;
								const char* unidade = "";
								 
                if (estado_atual == MENU_TAXA) {
                    ptr_indice = &indice_taxa; titulo = "Taxa Amostragem"; valor = valores_taxa[*ptr_indice];unidade = "kHz"; max_indice = 3;
                } else if (estado_atual == MENU_VDIV) {
                    ptr_indice = &indice_vdiv; titulo = "Escala Vertical"; valor = valores_vdiv[*ptr_indice];unidade = "mV/div";
                } else if (estado_atual == MENU_HDIV) {
                    ptr_indice = &indice_hdiv; titulo = "Escala de Tempo"; valor = valores_hdiv[*ptr_indice];unidade = "us/div";max_indice = 3;
                } else if (estado_atual == MENU_NIVEL_TRIG) {
                    ptr_indice = &indice_nivel_trig; titulo = "Nivel Trigger"; valor = valores_nivel[*ptr_indice];unidade = "mV"; max_indice = 3;
                } else if (estado_atual == MENU_BORDA_TRIG) {
                    ptr_indice = &indice_borda_trig; titulo = "Borda Trigger"; valor = (*ptr_indice);unidade = str_borda_trig[*ptr_indice];
                    max_indice = 1;
                } else if (estado_atual == MENU_MODO_SINGLE) {
	                    ptr_indice = &indice_modo_single; valor = (*ptr_indice);unidade = str_modo_single[*ptr_indice];max_indice =1;
	                }
								
								tx_mutex_get(&display_mutex, TX_WAIT_FOREVER);
                DesenharSubMenu(titulo, valor,unidade);
								tx_mutex_put(&display_mutex);
								
                // Navega para Esquerda / Direita (Altera o valor)
                if (joy_x < 1000 && *ptr_indice > 0) { (*ptr_indice)--; }
                if (joy_x > 3000 && *ptr_indice < max_indice) { (*ptr_indice)++; }
								

                if (clicou) {
                    
	                if (estado_atual == MENU_TAXA) {
	                    ptr_indice = &indice_taxa; valor = valores_taxa[*ptr_indice];onda.taxa_khz = valor;AtualizarTaxaAmostragem(onda.taxa_khz * 1000);
	                } else if (estado_atual == MENU_VDIV) {
	                    ptr_indice = &indice_vdiv;  valor = valores_vdiv[*ptr_indice];onda.v_div= valor;
	                } else if (estado_atual == MENU_HDIV) {
	                    ptr_indice = &indice_hdiv; valor = valores_hdiv[*ptr_indice];onda.h_div = valor;
	                } else if (estado_atual == MENU_NIVEL_TRIG) {
	                    ptr_indice = &indice_nivel_trig;  valor = valores_nivel[*ptr_indice];onda.nivel= valor;
	                } else if (estado_atual == MENU_BORDA_TRIG) {
	                    ptr_indice = &indice_borda_trig; valor = (*ptr_indice);unidade = str_borda_trig[*ptr_indice];onda.borda_trigger = valor;
	                } else if (estado_atual == MENU_MODO_SINGLE) {
	                    ptr_indice = &indice_modo_single; valor = (*ptr_indice);unidade = str_modo_single[*ptr_indice];onda.modo_single = valor;RearmarSingleShot();
	                }
					estado_atual = TELA_PRINCIPAL; 
                }
            }

            tx_thread_sleep(15); 
        }
    }
}