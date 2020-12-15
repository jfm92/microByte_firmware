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
#include "display_HAL.h"
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

char save_rom_dir[300];

uint16 color[PALETTE_SIZE];
uint8_t *framebuffer[2];
volatile uint8_t currentFramebuffer = 0;

uint32_t *audioBuffer[2];
volatile uint8_t audioBuffer_num = 0;
volatile unsigned char *audioBuffer_ptr;
volatile uint16_t audioBufferCount = 0;

bool GAME_GEAR = false;

static const char *TAG = "SMS_manager";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void SMS_start(){
    
    // Queue creation
    vidQueue = xQueueCreate(7, sizeof(uint16_t *));
    audioQueue = xQueueCreate(1, sizeof(uint32_t *));


    //Execute emulator tasks.
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024 * 4, NULL, 1, &videoTask_handler, 1);
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 1, &audioTask_handler, 1);
    xTaskCreatePinnedToCore(&SMSTask, "SMSTask", 3048, NULL, 1, &SMSTask_handler, 0);
}

void SMS_resume(){
    ESP_LOGI(TAG,"SMS Resume");
    vTaskResume(videoTask_handler);
    vTaskResume(audioTask_handler);
    vTaskResume(SMSTask_handler);
}

void SMS_suspend(){
    ESP_LOGI(TAG,"SMS Suspend");
    vTaskSuspend(SMSTask_handler);
    vTaskSuspend(videoTask_handler);
    vTaskSuspend(audioTask_handler);
}

void SMS_save_game(){
    
    //The save file direction should be added previously when you start a new game.
	FILE *fd = fopen(save_rom_dir, "w");

    if(fd != NULL){
        system_save_state(fd);
        ESP_LOGI(TAG,"Game save successded.");  
    }
    else{
        ESP_LOGE(TAG,"Error creating save game file.");
    }
    fclose(fd);   
}

bool SMS_load_game(const char *game_name, uint8_t console){

    if(console == SMS) ESP_LOGI(TAG,"Loading Sega Master System ROM: %s",game_name);
    else{
        ESP_LOGI(TAG,"Loading Sega Game Gear ROM: %s",game_name);
        GAME_GEAR = true;
    }

    //Load game ROM from the SD card.
    if(!load_rom(game_name, console)){
        ESP_LOGE(TAG,"Fail loading game.");
        return false;
    }

    //If exists a save game file for this game, open it.
    if(!GAME_GEAR) sprintf(save_rom_dir,"/sdcard/Master_System/Save_Data/%s.sav",game_name);
	else{
		sprintf(save_rom_dir,"/sdcard/Game_Gear/Save_Data/%s.sav",game_name);
	}

    FILE *fd = fopen(save_rom_dir, "r");

    if(fd != NULL){
        //TODO: Implement properly save state
        ESP_LOGI(TAG,"Found save game file of the ROM: %s",game_name);
        //system_load_state(fd);
    }
    else{
        ESP_LOGW(TAG,"Any save game available for this ROM.");
    }
    fclose(fd);

    return true;

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void videoTask(void *arg){
    ESP_LOGI(TAG, "SMS Video Task Initialize");

    uint8_t *param;
    display_HAL_SMS_frame(NULL,NULL,GAME_GEAR);
    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);
        render_copy_palette(color);
        display_HAL_SMS_frame(param,color,GAME_GEAR);
        xQueueReceive(vidQueue, &param, portMAX_DELAY);
        
    }

}

static void audioTask(void *arg){
    ESP_LOGI(TAG, "SMS Audio Task Initialize");

    uint32_t *param;

    while(1){
        xQueuePeek(audioQueue, &param, portMAX_DELAY);
        audio_submit((short *)param, snd.sample_count);
        xQueueReceive(audioQueue, &param, portMAX_DELAY);
    }
}

static void SMSTask(void *arg){

    ESP_LOGI(TAG, "SMS Task Init");

    ESP_LOGI(TAG,"Triying to allocated frame buffer on DMA memory.");
    
    //Allocating frame buffer
    framebuffer[0] = heap_caps_malloc(256 * 192,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );
    framebuffer[1] = heap_caps_malloc(256 * 192,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );

    if(framebuffer[0] == NULL){
        ESP_LOGW(TAG,"framebuffer[0] not enough DMA memory for allocate. \n Allocating on regular memory.");
        framebuffer[0] = malloc(256 * 192);

        if(framebuffer[0] == NULL){
            //If the framebuffer was not possible to allocated, it doesn't have sense to continue.
            ESP_LOGE(TAG,"framebuffer[0] regular allocation error, abort emulator run.");
            abort();
        }
    }

    if(framebuffer[1] == NULL){
        ESP_LOGW(TAG,"framebuffer[1] not enough DMA memory for allocate. \n Allocating on regular memory.");
        framebuffer[1] = malloc(256 * 192);

        if(framebuffer[1] == NULL){
            ESP_LOGE(TAG,"framebuffer[1] regular allocation error, abort emulator run.");
            abort();
        }
    }
    ESP_LOGI(TAG,"framebuffer allocated successfully.\nDisplayBuffer[0]:%p\nDisplayBuffer[1]:%p",framebuffer[0], framebuffer[1]);


    memset(framebuffer[0],0,256 * 192);
    memset(framebuffer[1],0,256 * 192);

    sms.use_fm = 0;

    //Set video configuration
    bitmap.width = 256;
    bitmap.height = 192;
    bitmap.pitch = bitmap.width;
    //bitmap.depth = 8;
    bitmap.data = framebuffer[0];

    set_option_defaults();

    option.sndrate = AUDIO_SAMPLE_RATE;
    option.overscan = 0;
    option.extra_gg = 0;
    option.bilinear = 0;
    option.aspect = 0;

    system_init2();
    system_reset();

    uint32 frame = 0;

    size_t bufferSize = snd.sample_count * 2 * sizeof(int16_t);

    audioBuffer[0] = heap_caps_malloc(bufferSize,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );
    audioBuffer[1] = heap_caps_malloc(bufferSize,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );

    if(audioBuffer[0] == NULL){
        ESP_LOGW(TAG,"audioBuffer[0] not enough DMA memory for allocate. \n Allocating on regular memory.");
        audioBuffer[0] = malloc(bufferSize);

        if(audioBuffer[0] == NULL){
            //If the framebuffer was not possible to allocated, it doesn't have sense to continue.
            ESP_LOGE(TAG,"audioBuffer[0] regular allocation error, abort emulator run.");
            abort();
        }
    }

    if(audioBuffer[1] == NULL){
        ESP_LOGW(TAG,"audioBuffer[1] not enough DMA memory for allocate. \n Allocating on regular memory.");
        audioBuffer[1] = malloc(bufferSize);

        if(audioBuffer[1] == NULL){
            ESP_LOGE(TAG,"audioBuffer[1] regular allocation error, abort emulator run.");
            abort();
        }
    }
    ESP_LOGI(TAG,"audioBuffer allocated successfully.\audioBuffer[0]:%p\audioBuffer[1]:%p",audioBuffer[0], audioBuffer[1]);

    audioBufferCount = snd.sample_count;


    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    while(1){
        
        startTime = xthal_get_ccount();
        input_set();
        //TODO: Coleco stuff

        if ((frame % 2) == 0){
            system_frame(0);
            xQueueSend(vidQueue, &bitmap.data, 0);

            currentFramebuffer = currentFramebuffer ? 0 : 1;
            bitmap.data = framebuffer[currentFramebuffer];
        }
        else{
            system_frame(1);
        }

        // Process audio
        for (int x = 0; x < snd.sample_count; x++){
             uint32_t sample;

            sample = (snd.output[0][x] << 16) + snd.output[1][x];
   
            audioBuffer[audioBuffer_num][x] = sample;
        }

        xQueueSend(audioQueue, &audioBuffer[audioBuffer_num], 0);
        audioBuffer_num = audioBuffer_num ? 0 : 1;

        stopTime = xthal_get_ccount();

        int elapsedTime;
        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;

        if (frame == 60){
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
            float fps = frame / seconds;

            printf("FPS:%f\n", fps);

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
}