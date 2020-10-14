#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"


#include "system_manager.h"


#include "display_hal.h"
#include "system_configuration.h"
#include "battery.h"
#include "sound_driver.h"
#include "user_input.h"
#include "GUI.h"


#include <esp_log.h>


#include "sd_storage.h"

#include "gnuboy_manager.h"
#include "NES_manager.h"
#include "SMS_manager.h"

#include "external_app.h"

//#include "bt_controller.h"


TaskHandle_t gui_handler;


static const char *TAG = "microByte_main";


void app_main(void){

    system_info();
    
    ESP_LOGI(TAG, "Memory Status:\r\n -SPI_RAM: %i Bytes\r\n -INTERNAL_RAM: %i Bytes\r\n -DMA_RAM: %i Bytes\r\n", \
    system_memory(MEMORY_SPIRAM),system_memory(MEMORY_INTERNAL),system_memory(MEMORY_DMA));

    /**************** Peripherals initialization **************/
    audio_init(16000);
    display_init();
    sd_init();
    
    input_init();
    battery_init();
   

    /**************** Message Queue initialization **************/
   
    batteryQueue = xQueueCreate(1, sizeof(struct BATTERY_STATUS));
    input_queue = xQueueCreate(10, sizeof(uint8_t));
    modeQueue = xQueueCreate(1, sizeof(struct SYSTEM_MODE));

    /**************** Tasks **************/
    xTaskCreatePinnedToCore(GUI_task, "Graphical User Interface", 1024*6, NULL, 1, &gui_handler, 0);
    xTaskCreatePinnedToCore(&batteryTask, "Battery management", 2048, NULL, 5, NULL, 0);

    bool game_running = false;
    bool game_executed = false;
    while(1){
        struct SYSTEM_MODE management;

        if( xQueueReceive(modeQueue, &management ,portMAX_DELAY)){
            switch(management.mode){

                case MODE_GAME:
                    if(management.status == 1){
                        if(management.console == GAMEBOY_COLOR){
                            vTaskSuspend(gui_handler);
                            gnuboy_start(management.game_name);
                            game_executed = true;
                            game_running=true;
                        }
                        else if(management.console == NES){
                            vTaskSuspend(gui_handler);
                            NES_start(management.game_name);
                            game_executed = true;
                            game_running=true;
                        }
                        else if(management.console == SMS){
                            vTaskSuspend(gui_handler);
                            SMS_start(management.game_name);
                            game_executed = true;
                            game_running=true;
                        }
                    }
                    else{
                        if(game_running && game_executed){
                            // TODO: Implement selection of console resumen suspend
                            gnuboy_suspend();
                            // To avoid noise whe is suspend the audio task, is necesary to clean the dma from previous data.
                            audio_terminate();
                            // Is necessary this delay to avoid bouncing between suspend and delay state.
                            vTaskDelay(250 / portTICK_RATE_MS);
                            vTaskResume(gui_handler);
                            // Refresh menu image
                            game_running=false;
                        }
                        else if(!game_running && game_executed){
                            vTaskSuspend(gui_handler);
                            // Is necessary this delay to avoid bouncing between suspend and delay state
                            vTaskDelay(250 / portTICK_RATE_MS);
                            gnuboy_resume();
                            game_running=true;
                        }

                    }
                break;

                case MODE_SAVE_GAME:
                    printf("Save Game\r\n");
                    //TODO: Implement saving system
                break;

                case MODE_LOAD_GAME:
                    printf("Load Game\r\n");
                    //TODO: Implement load system
                break;

                case MODE_EXT_APP:
                    ESP_LOGI(TAG, "Loading external App");
                    external_app_init(management.game_name);
                    esp_restart();
                break;

                case MODE_BATTERY_ALERT:
                break;

            
                case MODE_OUT:
                    //If we close a game, the wifi manager or the bluetooth controller is required to restart
                    esp_restart();
                break;
            }
        }
        
        
    }

    //led_error();
  
}