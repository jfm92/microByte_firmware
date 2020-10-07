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

/**********************
 *  STATIC PROTOTYPES
 **********************/


/**********************
 *  TASK HANDLERS
 **********************/

/**********************
 *  QUEUE HANDLERS
 **********************/


/**********************
 *   GLOBAL VARIABLES
 **********************/

static const char *TAG = "SMS_manager";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void SMS_start(const char *game_name){
    
    ESP_LOGI(TAG,"Executing Sega Master System game: %s",game_name);
    // Queue creation
   // vidQueue = xQueueCreate(1, sizeof(uint16_t *));
   // audioQueue = xQueueCreate(1, sizeof(uint16_t *));

    // Load game from the SD card and save on RAM.
    //gbc_rom_load(game_name);
    load_rom("sonic.sms");
   
    //Execute emulator tasks.
   // xTaskCreatePinnedToCore(&videoTask, "videoTask", 1024*2, NULL, 1, &videoTask_handler, 1);
   // xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 1, &audioTask_handler, 1);
   // xTaskCreatePinnedToCore(&gnuBoyTask, "gnuboyTask", 3048, NULL, 1, &gnuBoyTask_handler, 0);
}

void SMS_resume(){

}

void SMS_suspend(){

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static void videoTask(void *arg){}

static void audioTask(void *arg){}

static void SMSTask(void *arg){}