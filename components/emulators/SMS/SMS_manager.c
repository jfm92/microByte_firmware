// TODO: Acuerdate de WRAM

/*********************
 *      LIBRARIES
 *********************/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "user_input.h"
#include "display_hal.h"
#include "system_configuration.h"
#include "system_manager.h"
#include "sound_driver.h"

#include "shared.h"

/*********************
 *      DEFINES
 *********************/
#define AUDIO_SAMPLE_RATE (16000)

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void audioTask(void *arg);
static void videoTask(void *arg);
static void SMSTask(void *arg);
static void input_set();


/**********************
 *  TASK HANDLERS
 **********************/
TaskHandle_t videoTask_handler;
TaskHandle_t audioTask_handler;
TaskHandle_t SMSTask_handler;

/**********************
 *  QUEUE HANDLERS
 **********************/
QueueHandle_t vidQueue;
QueueHandle_t audioQueue;

/**********************
 *   GLOBAL VARIABLES
 **********************/

uint16 color[PALETTE_SIZE];
uint8_t *framebuffer[2];
int currentFramebuffer = 0;

uint32_t *audioBuffer = NULL;
int audioBufferCount = 0;

static const char *TAG = "SMS_manager";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void SMS_start(const char *game_name){
    
    ESP_LOGI(TAG,"Executing Sega Master System game: %s",game_name);

    // Queue creation
    vidQueue = xQueueCreate(1, sizeof(uint16_t *));
    audioQueue = xQueueCreate(1, sizeof(uint16_t *));

    // Load game from the SD card and save on RAM.
    load_rom(game_name);

    //Execute emulator tasks.
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 1, NULL, 1);
   // xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 1, &audioTask_handler, 1);
    xTaskCreatePinnedToCore(&SMSTask, "SMSTask", 3048, NULL, 1, &SMSTask_handler, 0);
}

void SMS_resume(){

}

void SMS_suspend(){

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void videoTask(void *arg){
    
    uint8_t *param;
    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);
        render_copy_palette(color);
        display_SMS_frame(param,color);
        xQueueReceive(vidQueue, &param, portMAX_DELAY);
    }

}

static void audioTask(void *arg){}

void system_manage_sram(uint8 *sram, int slot, int mode)
{
    printf("system_manage_sram\n");
    //sram_load();
}

static void SMSTask(void *arg){

    ESP_LOGI(TAG, "SMS Task Init");
    printf("Free init %i bytes\r\n",esp_get_free_heap_size());
    framebuffer[0] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT ) ;
    framebuffer[1] = heap_caps_malloc(256 * 192, MALLOC_CAP_8BIT);

    if(framebuffer[0] == 0 || framebuffer[1] == 0){
        ESP_LOGE(TAG, "Framebuffer allocation fail: frambuffer[0]=%p  [1]=%p",framebuffer[0],framebuffer[1]);
        abort();
    }

    memset(framebuffer[0],0,256 * 192);
    memset(framebuffer[1],0,256 * 192);

    sms.use_fm = 0;

    bitmap.width = 256;
    bitmap.height = 192;
    bitmap.pitch = bitmap.width;
    //bitmap.depth = 8;
    bitmap.data = framebuffer[0];

    set_option_defaults();

    option.sndrate = AUDIO_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;

    system_init2();
    system_reset();

    //LoadState(cartName);

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;

    while(1){
        startTime = xthal_get_ccount();
        input_set();
        //TODO: Coleco stuff

        if (0 || (frame % 2) == 0){
            system_frame(0);

            xQueueSend(vidQueue, &bitmap.data, portMAX_DELAY);

            currentFramebuffer = currentFramebuffer ? 0 : 1;
            bitmap.data = framebuffer[currentFramebuffer];
        }
        else
        {
            system_frame(1);
        }

        /*if (!audioBuffer || audioBufferCount < snd.sample_count)
        {
            if (audioBuffer)
                free(audioBuffer);

            size_t bufferSize = snd.sample_count * 2 * sizeof(int16_t);
            //audioBuffer = malloc(bufferSize);
            audioBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
            if (!audioBuffer)
                abort();

            audioBufferCount = snd.sample_count;

            printf("app_main: Created audio buffer (%d bytes).\n", bufferSize);
        }

        // Process audio
        for (int x = 0; x < snd.sample_count; x++)
        {
            uint32_t sample;

            if (muteFrameCount < 60 * 2)
            {
                // When the emulator starts, audible poping is generated.
                // Audio should be disabled during this startup period.
                sample = 0;
                ++muteFrameCount;
            }
            else
            {
                sample = (snd.output[0][x] << 16) + snd.output[1][x];
            }

            audioBuffer[x] = sample;
        }*/

        //audio_submit((short *)audioBuffer, snd.sample_count - 1);

        stopTime = xthal_get_ccount();

        int elapsedTime;
        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;

        if (frame == 60)
        {
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
            float fps = frame / seconds;

            printf("HEAP:0x%x, FPS:%f\n", esp_get_free_heap_size(), fps);

            frame = 0;
            totalElapsedTime = 0;
        }

    }
    
}

static void input_set(){
    int smsButtons = 0;
    int smsSystem = 0;

    uint16_t inputs_value =  input_read();

    if(!((inputs_value >> 0) & 0x01))  smsButtons |= INPUT_DOWN;
    if(!((inputs_value >> 1) & 0x01))  smsButtons |= INPUT_LEFT;
    if(!((inputs_value >> 2) & 0x01))  smsButtons |= INPUT_UP;
    if(!((inputs_value >> 3) & 0x01))  smsButtons |= INPUT_RIGHT;
    if(!((inputs_value >> 8) & 0x01))  smsButtons |= INPUT_BUTTON1;
    if(!((inputs_value >> 9) & 0x01))  smsButtons |= INPUT_BUTTON2;  

    if(!((inputs_value >> 10) & 0x01))  smsSystem |= INPUT_START;
    if(!((inputs_value >> 12) & 0x01))  smsSystem |= INPUT_PAUSE;

    input.pad[0] = smsButtons;
    input.system = smsSystem;

    if(!((inputs_value >>11) & 0x01)){
        struct SYSTEM_MODE emulator;
            emulator.mode = MODE_GAME;
            emulator.status = 0;
     
            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"On game menu queue send fail");
            }
    }
}