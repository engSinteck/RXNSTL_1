// ============================================================================
// wav_player_safe.c - Versão segura sem MemManage
// ============================================================================

#include "main.h"
#include "fatfs.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "i2s.h"
#include "fatfs.h"
#include <string.h>
#include <math.h>

#include "wav_player.h"

// ============================================================================
// CONFIGURAÇÕES - Tamanhos reduzidos para teste
// ============================================================================
#define SAMPLE_RATE         48000
#define SAFE_BUFFER_SAMPLES 4096    // REDUZIDO para 256 samples (teste)
#define SAFE_HALF_WORDS     (SAFE_BUFFER_SAMPLES * 2)   // 8192 words
#define SAFE_TOTAL_WORDS    (SAFE_HALF_WORDS * 2)       // 16384 words

// Verificação em tempo de compilação
#if SAFE_TOTAL_WORDS > 16384
    #error "Buffer muito grande"
#endif

static float sine_phase = 0.0f;
const float SINE_FREQ = 1000.0f;
const float SAMPLE_RATE_F = 48000.0f;

// ============================================================================
// BUFFER ÚNICO CONTÍGUO - Forçado para endereço específico e alinhado
// ============================================================================
// Usar atributos específicos para garantir alinhamento de cache (32 bytes)
static int32_t audio_buffer[SAFE_TOTAL_WORDS] __attribute__((section(".RAM_D2"), aligned(32)));

// Buffer temporário
static uint8_t pcm_buffer[8192] __attribute__((section(".RAM_D2"), aligned(32)));  // 2048 bytes = 512 samples

BaseType_t xTaskAudio;

// Verificar se o buffer está em região acessível pelo DMA
// RAM_D2 é 0x30000000 (acessível por todos DMAs no H7)

// ============================================================================
// ESTRUTURA DO PLAYER COM VALIDAÇÕES
// ============================================================================
typedef struct {
    FIL file;
    uint32_t data_remaining;
    uint8_t is_playing;
    uint8_t file_open;
    uint32_t bytes_read_total;
    uint32_t buffer_size_words;   // Guarda tamanho do buffer para validação
} wav_player_t;

static wav_player_t player = {0};
static TaskHandle_t player_task_handle = NULL;

// Semáforos
static SemaphoreHandle_t half_semaphore = NULL;
static SemaphoreHandle_t full_semaphore = NULL;

// ============================================================================
// FUNÇÃO SEGURA DE LEITURA COM VALIDAÇÃO DE LIMITES
// ============================================================================
static uint32_t read_and_convert_wav(int32_t* buffer, uint32_t max_words)
{
    // VALIDAÇÕES DE SEGURANÇA
    if(!player.is_playing || !player.file_open) {
        return 0;
    }

    if(buffer == NULL) {
        //printf("ERRO: buffer NULL\n");
        return 0;
    }

    if(max_words == 0 || max_words > SAFE_HALF_WORDS) {
        //("ERRO: max_words inválido: %lu\n", max_words);
        return 0;
    }

    if(player.data_remaining == 0) {
        return 0;
    }

    // Calcula bytes a ler (max_frames = max_words / 2)
    uint32_t max_frames = max_words / 2;
    uint32_t bytes_needed = max_frames * 4;
    uint32_t bytes_to_read = (bytes_needed > player.data_remaining) ?
                              player.data_remaining : bytes_needed;

    // Limite adicional de segurança
    if(bytes_to_read > 4096) {
        bytes_to_read = 4096;
    }

    if(bytes_to_read == 0) {
        return 0;
    }

    if(bytes_to_read > sizeof(pcm_buffer)) {
        //printf("ERRO: bytes_to_read muito grande: %lu\n", bytes_to_read);
        bytes_to_read = sizeof(pcm_buffer);
    }

    // LEITURA DO ARQUIVO
    UINT bytes_read = 0;
    FRESULT result = f_read(&player.file, pcm_buffer, bytes_to_read, &bytes_read);

    if(result != FR_OK) {
        //printf("f_read error: %d\n", result);
        return 0;
    }

    if(bytes_read == 0) {
        return 0;
    }

    // CONVERSÃO SEGURA (com verificação de limites)
    uint16_t* pcm16 = (uint16_t*)pcm_buffer;
    uint32_t frames_read = bytes_read / 4;

    // Garantir que não escrevemos fora do buffer
    if(frames_read > max_frames) {
        frames_read = max_frames;
    }

    // Limpa o buffer antes de preencher (evita lixo)
    //memset(buffer, 0, max_words * 4);

    for(uint32_t i = 0; i < frames_read; i++) {
        // Verificação de segurança dos índices
        if((i * 2 + 1) >= max_words) {
            //printf("ERRO: índice fora do buffer: i=%lu, max_words=%lu\n", i, max_words);
            break;
        }

        int16_t left = (int16_t)pcm16[i * 2];
        int16_t right = (int16_t)pcm16[i * 2 + 1];

        // Mesmo formato do teste que funcionou
        buffer[i * 2]     = ((int32_t)left) << 16;
        buffer[i * 2 + 1] = ((int32_t)right) << 16;
    }

    player.data_remaining -= bytes_read;
    player.bytes_read_total += bytes_read;

    // Debug periódico
    static uint32_t debug_counter = 0;
    if(debug_counter++ % 100 == 0) {
        //printf("Progresso: %lu bytes lidos, %lu restantes\n",
        //       player.bytes_read_total, player.data_remaining);
    }

    return frames_read;
}

// ============================================================================
// CALLBACKS DO DMA (COM VERIFICAÇÃO)
// ============================================================================
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Toggle GPIO
    HAL_GPIO_TogglePin(LED_LOCK_GPIO_Port, LED_LOCK_Pin);

    if(half_semaphore != NULL) {
        xSemaphoreGiveFromISR(half_semaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // Toggle GPIO
    HAL_GPIO_TogglePin(LED_RSSI_GPIO_Port, LED_RSSI_Pin);

    if(full_semaphore != NULL) {
        xSemaphoreGiveFromISR(full_semaphore, &xHigherPriorityTaskWoken);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ============================================================================
// TASK DO PLAYER (COM TRATAMENTO DE ERROS)
// ============================================================================
static void vAudioPlayerTask(void *pvParameters)
{
    uint32_t frames_written;
    //uint32_t half_frames = SAFE_BUFFER_SAMPLES;

    //printf("\n=== Audio Player Task Iniciada ===\n");
    //printf("Buffer size: %d words (%d bytes)\n", SAFE_TOTAL_WORDS, sizeof(audio_buffer));
    //printf("Half frames: %lu\n", half_frames);

    // ========================================================================
    // PRÉ-CARREGAMENTO DOS BUFFERS
    // ========================================================================
    //printf("Pré-carregando primeira metade...\n");
    frames_written = read_and_convert_wav(&audio_buffer[0], SAFE_HALF_WORDS);
    //printf("  -> %lu frames escritos\n", frames_written);

    //printf("Pré-carregando segunda metade...\n");
    frames_written = read_and_convert_wav(&audio_buffer[SAFE_HALF_WORDS], SAFE_HALF_WORDS);
    //printf("  -> %lu frames escritos\n", frames_written);

    // ========================================================================
    // LIMPA CACHE (CRÍTICO PARA H7)
    // ========================================================================
    //printf("Limpando DCache...\n");
    SCB_CleanDCache_by_Addr((uint32_t*)audio_buffer, sizeof(audio_buffer));

    // ========================================================================
    // INICIA DMA
    // ========================================================================
   // printf("Iniciando DMA...\n");
    HAL_StatusTypeDef status = HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t*)audio_buffer, SAFE_TOTAL_WORDS);

    if(status != HAL_OK) {
       // printf("ERRO ao iniciar DMA: %d\n", status);
        player.is_playing = 0;
        vTaskDelete(NULL);
        return;
    }

    //printf("DMA iniciado com sucesso! Tocando...\n");

    // ========================================================================
    // LOOP PRINCIPAL
    // ========================================================================
    while(player.is_playing) {
    	// Processa Metade Inicial
    	if(xSemaphoreTake(half_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
    		//frames_written = read_and_convert_wav(&audio_buffer[0], SAFE_HALF_WORDS);
    		fill_sine_buffer_task(&audio_buffer[0], SAFE_HALF_WORDS);

    		// Garante que o dado saiu da CPU para a RAM antes do DMA ler
    		SCB_CleanDCache_by_Addr((uint32_t*)&audio_buffer[0], SAFE_HALF_WORDS * 4);

    	    if(frames_written == 0) break;
    	}

    	// Processa Metade Final
    	if(xSemaphoreTake(full_semaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
    		//frames_written = read_and_convert_wav(&audio_buffer[SAFE_HALF_WORDS], SAFE_HALF_WORDS);
    		fill_sine_buffer_task(&audio_buffer[SAFE_HALF_WORDS], SAFE_HALF_WORDS);

    		// Garante que o dado saiu da CPU para a RAM antes do DMA ler
    	    SCB_CleanDCache_by_Addr((uint32_t*)&audio_buffer[SAFE_HALF_WORDS], SAFE_HALF_WORDS * 4);

    	    if(frames_written == 0) break;
    	}
    }

    // ========================================================================
    // FINALIZAÇÃO
    // ========================================================================
   // printf("Playback finalizado. Total lido: %lu bytes\n", player.bytes_read_total);

    Audio_Player_Stop();
    vTaskDelete(NULL);
    player_task_handle = NULL;
}

// ============================================================================
// FUNÇÕES PÚBLICAS
// ============================================================================
void Audio_Player_Init(void)
{
    //printf("\n=== Inicializando Audio Player ===\n");

    // Cria semáforos
    half_semaphore = xSemaphoreCreateBinary();
    full_semaphore = xSemaphoreCreateBinary();

    if(half_semaphore == NULL || full_semaphore == NULL) {
       // printf("ERRO: Falha ao criar semáforos\n");
        return;
    }

    // Limpa buffer
    memset(audio_buffer, 0, sizeof(audio_buffer));
    SCB_CleanDCache_by_Addr((uint32_t*)audio_buffer, sizeof(audio_buffer));

   // printf("Audio Player inicializado com sucesso\n");
   // printf("Buffer em: 0x%08lX, tamanho: %u bytes\n",
   //        (uint32_t)audio_buffer, sizeof(audio_buffer));

   player_task_handle = NULL;
}

void Audio_Player_Start(const char* filename)
{
   // printf("\n=== Iniciando playback ===\n");
   // printf("Arquivo: %s\n", filename);

    // Para playback anterior
    if(player.is_playing) {
        Audio_Player_Stop();
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Abre arquivo
    FRESULT result = f_open(&player.file, filename, FA_READ);
    if(result != FR_OK) {
        //printf("ERRO: Não foi possível abrir %s (código: %d)\n", filename, result);
        return;
    }

    // Pula header WAV (assume 44 bytes)
    f_lseek(&player.file, 44);

    // Inicializa estrutura do player
    player.data_remaining = f_size(&player.file) - 44;
    player.file_open = 1;
    player.is_playing = 1;
    player.bytes_read_total = 0;
    player.buffer_size_words = SAFE_TOTAL_WORDS;

    //("Arquivo aberto: %lu bytes de dados de áudio\n", player.data_remaining);
    //printf("Sample rate esperado: 48000 Hz\n");

    // Cria task do player
    if(player_task_handle == NULL) {
    	xTaskAudio = xTaskCreate(vAudioPlayerTask, "AudioPlayer", 1024, NULL, osPriorityRealtime, &player_task_handle);
    }
    printf("Task Audio: %ld\n", xTaskAudio);
}

void Audio_Player_Stop(void)
{
    //printf("Parando playback...\n");

    player.is_playing = 0;

    if(player.file_open) {
        f_close(&player.file);
        player.file_open = 0;
    }

    // Para o DMA
    HAL_I2S_DMAStop(&hi2s2);

    // Limpa buffer
    memset(audio_buffer, 0, sizeof(audio_buffer));
    SCB_CleanDCache_by_Addr((uint32_t*)audio_buffer, sizeof(audio_buffer));
}

uint8_t Audio_Player_IsPlaying(void)
{
    return player.is_playing;
}

// ============================================================================
// HANDLER DO MemManage Fault (para debug)
// ============================================================================
void MemManage_Handler(void)
{
    printf("\n!!! MEM MANAGE FAULT !!!\n");
    printf("SCB_MMFAR: 0x%08lX\n", SCB->MMFAR);
    printf("SCB_CFSR: 0x%08lX\n", SCB->CFSR);

    // Desliga o player para evitar loops
    Audio_Player_Stop();

    while(1);
}

void fill_sine_buffer_task(int32_t* buffer, uint32_t num_words)
{
    // num_words é o total de uint32_t (4096 para meia transferência)
    // Como é stereo, processamos num_words / 2 frames
    uint32_t num_frames = num_words / 2;

    for (uint32_t i = 0; i < num_frames; i++)
    {
        // Calcula o valor da amostra
        float val = sinf(sine_phase);
        int32_t sample = (int32_t)(val * 0.5f * 2147483647.0f); // Volume em 50% para evitar clipping

        // No seu I2S 32-bit Philips:
        buffer[2 * i]     = sample; // Canal Esquerdo
        buffer[2 * i + 1] = sample; // Canal Direito

        // Incrementa a fase e mantém entre 0 e 2*PI
        sine_phase += (2.0f * M_PI * SINE_FREQ) / SAMPLE_RATE_F;
        if (sine_phase >= (2.0f * M_PI)) {
            sine_phase -= (2.0f * M_PI);
        }
    }
}
