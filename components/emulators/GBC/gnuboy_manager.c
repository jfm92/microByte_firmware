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

// GNUBoy libraries

#include <loader.h>
#include <hw.h>
#include <lcd.h>
#include <fb.h>
#include <cpu.h>
#include <pcm.h>
#include <regs.h>
#include <rtc.h>
#include <gnuboy.h>
#include <sound.h>

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void audioTask(void *arg);
static void run_to_vblank();
static void videoTask(void *arg);
static void gnuBoyTask(void *arg);
static void input_set();

/**********************
 *  TASK HANDLERS
 **********************/
TaskHandle_t videoTask_handler;
TaskHandle_t audioTask_handler;
TaskHandle_t gnuBoyTask_handler;

/**********************
 *  QUEUE HANDLERS
 **********************/
QueueHandle_t vidQueue;
QueueHandle_t audioQueue;

/**********************
 *   GLOBAL VARIABLES
 **********************/

bool game_running = false;
static const char *TAG = "gnuBoy_manager";



struct fb fb;
struct pcm pcm;

uint16_t *displayBuffer[2];
uint8_t currentBuffer;

uint16_t *framebuffer;
int frame = 0;
uint elapsedTime = 0;

volatile bool videoTaskIsRunning = false;


////////////////////////////////////////////////
unsigned char *audioBuffer[2];
volatile uint8_t currentAudioBuffer = 0;
volatile uint16_t currentAudioSampleCount;
volatile unsigned char *currentAudioBufferPtr;

char * game_name;
uint8_t console_use;

#define AUDIO_SAMPLE_RATE (16000)

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void gnuboy_start(){
    
    // Queue creation
    vidQueue = xQueueCreate(7, sizeof(uint16_t *));
    audioQueue = xQueueCreate(1, sizeof(uint16_t *));

    //Execute emulator tasks.
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024*2, NULL, 1, &videoTask_handler, 0);
    xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 1, &audioTask_handler, 0);
    xTaskCreatePinnedToCore(&gnuBoyTask, "gnuboyTask", 3048, NULL, 5, &gnuBoyTask_handler, 1);

}

void gnuboy_resume(){
    ESP_LOGI(TAG,"GameBoy Color Resume");
    vTaskResume(videoTask_handler);
    vTaskResume(audioTask_handler);
    vTaskResume(gnuBoyTask_handler);
}

void gnuboy_suspend(){
    ESP_LOGI(TAG,"GameBoy Color Suspend");
    vTaskSuspend(gnuBoyTask_handler);
    vTaskSuspend(videoTask_handler);
    vTaskSuspend(audioTask_handler);
}

/** 
 * TODO: Improve game save.
 * Basically it works, but when you try to save every 30 seconds, the SD card crashed so, it corrupt 
 * the file save. On the other hand I notice that some texture dissapear when the game is saved.
 * */

void gnuboy_save(){
    gbc_state_save(game_name, console_use);
}

bool gnuboy_load_game(const char *name, uint8_t console){

    ESP_LOGI(TAG,"Loading GameBoy Color game: %s",name);

    game_name = malloc(strlen(name));
    strcpy(game_name,name);

    console_use = console;

    // Load game from the SD card and save on RAM.
    if(!gbc_rom_load(game_name,console)){
        ESP_LOGE(TAG,"Error loading game: %s",game_name);
        return false;
    }
    if(!gbc_state_load(game_name,console)) ESP_LOGW(TAG,"Error loading save game, starting new save game.");

    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void videoTask(void *arg){

    ESP_LOGI(TAG, "GNUBoy Video Task Initialize");
    uint16_t *param;

    //Send empty frame
    display_HAL_gb_frame(NULL);
    
    while(1){
        xQueuePeek(vidQueue, &param, portMAX_DELAY);
        display_HAL_gb_frame(param);
        xQueueReceive(vidQueue, &param, portMAX_DELAY);
    }

    ESP_LOGE(TAG,"GNUBoy video task fatal error");
    vTaskDelete(NULL);
}

static void audioTask(void *arg){

    ESP_LOGI(TAG, "GNUBoy Audio Task Initialize");
    uint16_t *param;

    while(1){
        xQueuePeek(audioQueue, &param, portMAX_DELAY);
        audio_submit((short *)param, currentAudioSampleCount >> 1);
        xQueueReceive(audioQueue, &param, portMAX_DELAY);
    }

    ESP_LOGE(TAG,"GNUBoy audio task fatal error");
    vTaskDelete(NULL);

}

static void gnuBoyTask(void *arg){

    ESP_LOGI(TAG, "Initialize GNUBoy task");

    ESP_LOGI(TAG,"Triying to allocated frame buffer on DMA memory.");
    displayBuffer[0] = heap_caps_malloc(160 * 144 * 2,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );
    displayBuffer[1] = heap_caps_malloc(160 * 144 * 2,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );

    if(displayBuffer[0] == NULL){
        ESP_LOGW(TAG,"DisplayBuffer[0] not enough DMA memory for allocate. \n Allocating on regular memory.");
        displayBuffer[0] = malloc(160 * 144 * 2);

        if(displayBuffer[0] == NULL){
            //If the framebuffer was not possible to allocated, it doesn't have sense to continue.
            ESP_LOGE(TAG,"DisplayBuffer[0] regular allocation error, abort emulator run.");
            abort();
        }
    }

    if(displayBuffer[1] == NULL){
        ESP_LOGW(TAG,"DisplayBuffer[1] not enough DMA memory for allocate. \n Allocating on regular memory.");
        displayBuffer[1] = malloc(160 * 144 * 2);

        if(displayBuffer[1] == NULL){
            ESP_LOGE(TAG,"DisplayBuffer[1] regular allocation error, abort emulator run.");
            abort();
        }
    }
    ESP_LOGI(TAG,"DisplayBuffer allocated successfully.\nDisplayBuffer[0]:%p\nDisplayBuffer[1]:%p",displayBuffer[0], displayBuffer[1]);

    //Clean the buffers
    memset(displayBuffer[0], 0, 160 * 144 * 2);
    memset(displayBuffer[1], 0, 160 * 144 * 2);
    
    emu_reset();

    //Set RTC configuration
    rtc.d = 1;
    rtc.h = 1;
    rtc.m = 1;
    rtc.s = 1;
    rtc.t = 1;

    // Emulator video configuration
    framebuffer = displayBuffer[0];
    memset(&fb, 0, sizeof(fb));
    fb.w = 160;
    fb.h = 144;
    fb.pelsize = 2;
    fb.pitch = fb.w * fb.pelsize;
    fb.indexed = 0;
    fb.ptr = framebuffer;
    fb.enabled = 1;
    fb.dirty = 0;

    //Audio configuration

    //Calculate audio buffer size
    const int audioBufferLength = AUDIO_SAMPLE_RATE / 10 + 1;
    const int AUDIO_BUFFER_SIZE = audioBufferLength * sizeof(int16_t) * 2;

    audioBuffer[0] = heap_caps_malloc(AUDIO_BUFFER_SIZE,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );
    audioBuffer[1] = heap_caps_malloc(AUDIO_BUFFER_SIZE,MALLOC_CAP_8BIT | MALLOC_CAP_DMA );

    if(audioBuffer[0] == NULL){
        ESP_LOGW(TAG,"audioBuffer[0] not enough DMA memory for allocate. \n Allocating on regular memory.");
        audioBuffer[0] = malloc(AUDIO_BUFFER_SIZE);

        if(audioBuffer[0] == NULL){
            //If the framebuffer was not possible to allocated, it doesn't have sense to continue.
            ESP_LOGE(TAG,"audioBuffer[0] regular allocation error, abort emulator run.");
            abort();
        }
    }

    if(audioBuffer[1] == NULL){
        ESP_LOGW(TAG,"audioBuffer[1] not enough DMA memory for allocate. \n Allocating on regular memory.");
        audioBuffer[1] = malloc(AUDIO_BUFFER_SIZE);

        if(audioBuffer[1] == NULL){
            ESP_LOGE(TAG,"audioBuffer[1] regular allocation error, abort emulator run.");
            abort();
        }
    }
    ESP_LOGI(TAG,"audioBuffer allocated successfully.\audioBuffer[0]:%p\audioBuffer[1]:%p",audioBuffer[0], audioBuffer[1]);

    memset(&pcm, 0, sizeof(pcm));
    pcm.hz = AUDIO_SAMPLE_RATE;
    pcm.stereo = 1;
    pcm.len =  audioBufferLength;
    pcm.buf = audioBuffer[0];
    pcm.pos = 0;

    gbc_sound_reset();

    lcd_begin();

    //Variables to get an aprox FPS count of the game
    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    uint actualFrameCount = 0;

    //TODO: This loop needs to be improved.
    while(1){
        startTime = xthal_get_ccount();
        //Render a frame with audio
        run_to_vblank();
        stopTime = xthal_get_ccount();

        //Get the status of the input buttons
        input_set();

        if (stopTime > startTime) elapsedTime = (stopTime - startTime);
        else elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        actualFrameCount++;
        frame++; //Increase the count of the frame to generate

        if (actualFrameCount == 60){
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
            float fps = actualFrameCount / seconds;

            printf("FPS:%f\n", fps);

            actualFrameCount = 0;
            totalElapsedTime = 0;
        }
        
    }
}


static void run_to_vblank(){
   //Frame and sound generation

    cpu_emulate(32832);

    while (R_LY > 0 && R_LY < 144) emu_step(); // Step through visible line scanning phase 

    //Yes, skipframe.
    if ((frame % 2) == 0)
    {
        xQueueSend(vidQueue, &framebuffer, 0);

        // swap buffers
        currentBuffer = currentBuffer ? 0 : 1;
        framebuffer = displayBuffer[currentBuffer];

        fb.ptr = framebuffer;
    }

    rtc_tick();

    //Generate the sound for each frame
    sound_mix();

    if (pcm.pos > 100){
        currentAudioBufferPtr = audioBuffer[currentAudioBuffer];
        currentAudioSampleCount = pcm.pos;

        xQueueSend(audioQueue, &currentAudioBufferPtr, portMAX_DELAY);

        // Swap buffers
        currentAudioBuffer = currentAudioBuffer ? 0 : 1;
        pcm.buf = audioBuffer[currentAudioBuffer];
        pcm.pos = 0;
    }

    if (!(R_LCDC & 0x80)) cpu_emulate(32832);

    while (R_LY > 0) emu_step(); // Step through vblank phase 
 
}

static void input_set(){

    uint16_t inputs_value =  input_read();

    pad_set(PAD_DOWN,!((inputs_value >> 0) & 0x01));
    pad_set(PAD_LEFT,!((inputs_value >> 1) & 0x01));
    pad_set(PAD_UP,!((inputs_value >> 2) & 0x01));
    pad_set(PAD_RIGHT,!((inputs_value >> 3) & 0x01));
    pad_set(PAD_B,!((inputs_value >> 8) & 0x01));
    pad_set(PAD_A,!((inputs_value >> 9) & 0x01));
    pad_set(PAD_START,!((inputs_value >> 10) & 0x01));
    pad_set(PAD_SELECT,!((inputs_value >> 12) & 0x01));

}

