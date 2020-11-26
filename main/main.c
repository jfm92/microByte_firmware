
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include <freertos/timers.h>


#include "system_manager.h"
#include "system_configuration.h"

#include <esp_log.h>

#include "gnuboy_manager.h"
#include "NES_manager.h"
#include "SMS_manager.h"

#include "external_app.h"
#include "update_firmware.h"
#include "boot_screen.h"
#include "display_HAL.h"
#include "sd_storage.h"
#include "battery.h"
#include "sound_driver.h"
#include "GUI.h"
#include "user_input.h"

uint8_t console_running = NULL;

TaskHandle_t gui_handler;
TaskHandle_t intro_handler;
TimerHandle_t timer;


static const char *TAG = "microByte_main";

void intro_taks(void *arg){
   ESP_LOGI(TAG,"Boot screen animation init.");
   boot_screen_task();
}

static void timer_isr(void){
    printf("save\r\n");
    //gnuboy_save();
    struct SYSTEM_MODE emulator;
    emulator.mode = MODE_SAVE_GAME;
   // emulator.console = emulator_selected;

    if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
        ESP_LOGE(TAG,"modeQueue send error");
    }
}

void app_main(void){
    system_info();

    ESP_LOGI(TAG, "Memory Status:\r\n -SPI_RAM: %i Bytes\r\n -INTERNAL_RAM: %i Bytes\r\n -DMA_RAM: %i Bytes\r\n", \
    system_memory(MEMORY_SPIRAM),system_memory(MEMORY_INTERNAL),system_memory(MEMORY_DMA));
    display_HAL_init();
    xTaskCreatePinnedToCore(boot_screen_task, "intro_task", 2048, NULL, 1, &intro_handler, 1);
    /**************** Peripherals initialization **************/
    
    audio_init(AUDIO_SAMPLE_16KHZ);
   // display_init();
    sd_init();
    input_init();
    battery_init();

    // After peripheral initialization we check if a new update was installed 
    // and if any hardware issue happend
    update_check();

   

    /**************** Message Queue initialization **************/
   
    batteryQueue = xQueueCreate(1, sizeof(struct BATTERY_STATUS));
    modeQueue = xQueueCreate(1, sizeof(struct SYSTEM_MODE));

    /**************** Tasks **************/
    xTaskCreatePinnedToCore(&batteryTask, "Battery management", 2048, NULL, 5, NULL, 0);
    printf("Finish intro\r\n");
    vTaskDelay(2500 / portTICK_RATE_MS);
    vTaskDelete(intro_handler);
    boot_screen_free();
    display_HAL_change_endian();
    xTaskCreatePinnedToCore(GUI_task, "Graphical User Interface", 1024*6, NULL, 1, &gui_handler, 0);
   

    bool game_running = false;
    bool game_executed = false;
    while(1){
        struct SYSTEM_MODE management;

        if( xQueueReceive(modeQueue, &management ,portMAX_DELAY)){
            switch(management.mode){

                case MODE_GAME:
                    if(management.status == 1){
                        battery_game_mode(true);

                        if(management.console == GAMEBOY_COLOR || management.console == GAMEBOY){
                            vTaskSuspend(gui_handler);
                            gnuboy_load_game(management.game_name,management.console);
                                
                                gnuboy_start();
                                game_executed = true;
                                game_running=true;
                                console_running = management.console;
                           /* timer = xTimerCreate("nes", pdMS_TO_TICKS( 30000 ), pdTRUE, NULL, timer_isr);
                            if(timer != pdPASS) xTimerStart(timer, 0);
                            else{
                                ESP_LOGE(TAG,"Save timer initialization fail.");
                            }*/
	                        
                            
                            
                        }
                        else if(management.console == NES){
                            vTaskSuspend(gui_handler);
                            NES_load_game(management.game_name);
                            NES_start();
                            game_executed = true;
                            game_running=true;
                            console_running = NES;
                        }
                        else if(management.console == SMS || management.console == GG){
                            vTaskSuspend(gui_handler);

                            SMS_load_game(management.game_name,management.console);
                            SMS_start();
                            game_executed = true;
                            game_running=true;
                            console_running = management.console;
                        }
                    }
                    else{
                        if(game_running && game_executed){
                           if(console_running == GAMEBOY_COLOR || console_running == GAMEBOY ) gnuboy_suspend();
                           else if(console_running == NES) NES_suspend();
                           else if(console_running == SMS || console_running == GG) SMS_suspend();

                            // To avoid noise whe is suspend the audio task, is necesary to clean the dma from previous data.
                            audio_terminate();
                            // Is necessary this delay to avoid bouncing between suspend and delay state.
                            vTaskDelay(250 / portTICK_RATE_MS);
                            vTaskResume(gui_handler);
                            GUI_refresh();
                            // Refresh menu image
                            game_running=false;
                        }
                        else if(!game_running && game_executed){
                            vTaskSuspend(gui_handler);
                            // Is necessary this delay to avoid bouncing between suspend and delay state
                            vTaskDelay(250 / portTICK_RATE_MS);
                            if(console_running == GAMEBOY_COLOR || console_running == GAMEBOY ) gnuboy_resume();
                           else if(console_running == NES) NES_resume();
                           else if(console_running == SMS || console_running == GG) SMS_resume();
                            game_running=true;
                        }

                    }
                break;

                case MODE_SAVE_GAME:

                        ESP_LOGI(TAG,"Saving data GameBoy Color");
                      //  gnuboy_save();
                      SMS_save_game();
                        printf("guardado\r\n");
                    
                break;

                case MODE_EXT_APP:
                    ESP_LOGI(TAG, "Loading external App");
                    vTaskDelay(1000 / portTICK_RATE_MS);
                    external_app_init(management.game_name);
                    display_HAL_clear();
                   //update_init(management.game_name);
                   esp_restart();
                   
                break;

                case MODE_BATTERY_ALERT:
                    //If in play mode, pause game and show if you wanna save
                    //If in the menu just show the message
                    gnuboy_suspend();
                    audio_terminate();
                            // Is necessary this delay to avoid bouncing between suspend and delay state.
                    vTaskDelay(250 / portTICK_RATE_MS);
                    vTaskResume(gui_handler);
                    GUI_async_message();
                    GUI_refresh();
                    game_running=false;
                break;

                case MODE_CHANGE_VOLUME:
                    audio_volume_set((float)management.volume_level);
                break;

                case MODE_CHANGE_BRIGHT:
                    //st7789_backlight_set(management.brightness_level);
                break;

                case MODE_UPDATE:
                    ESP_LOGI(TAG,"Update firmware");

                    if(battery_get_percentage() >= 70){
                        update_init(management.game_name);
                        esp_restart();
                    }
                    else{
                        ESP_LOGI(TAG, "Battery level less than 70 percent. Is not possible to update");
                        //TODO: Show message on the GUI with this error
                    }
                    
                break;

                case MODE_OUT:
                    esp_restart();
                break;
            }
        }
        
        
    }
}