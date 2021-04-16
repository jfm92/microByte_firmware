
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
#include "LED_notification.h"
#include "backlight_ctrl.h"

uint8_t console_running = NULL;

TaskHandle_t gui_handler;
TaskHandle_t intro_handler;
TimerHandle_t timer;

bool boot_screen_ani = true;


static const char *TAG = "microByte_main";


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

/********************************************************
 * Greetings code explorer, welcome to the microByte
 * firmware. If you want to understand this firmware and/or
 * modified it, here starts your journey.
 * 
 * This is the main loop of the firmware initialize the system 
 * resources and manage it. 
 * 
 * My plan is to create a block diagram to have a better
 * understanding of the code, but you know, the time is 
 * precious resource that I don't have nowadays.
 * 
 * On deeper layer of the code I won't give this quantity of 
 * commentary, because I know that this is too much, but 
 * also I think is good to have a easy start. 
 * *****************************************************/

void app_main(void){

    /**************** Basic initialization **************/
    system_init_config();
    //And the light was done. Initialize the LED control thread and perfom an "fade animation"
    LED_init();
    LED_mode(LED_FADE_ON); //There are a few animations available to choose.

    //This is just for debugging pourposes. This MCU is a little weird in terms of memory management. 
    system_info();
    ESP_LOGI(TAG, "Memory Status:\r\n -SPI_RAM: %i Bytes\r\n -INTERNAL_RAM: %i Bytes\r\n -DMA_RAM: %i Bytes\r\n", \
    system_memory(MEMORY_SPIRAM),system_memory(MEMORY_INTERNAL),system_memory(MEMORY_DMA));

    //Start the display Hardware Abstraction Layer (Basically just initi the display). This layer is just to simplify the port process if you want to use 
    // a different display. 
    display_HAL_init();

    //This initialize the display's backlight control. This is a dirty backligh control module, I plan to change in a near future.
    backlight_init();

    int8_t backlight_level = system_get_config(SYS_BRIGHT);
    if(backlight_level > -1) backlight_set(backlight_level);

    /**************** Boot status **************/

    //Get the previous system state.
    int32_t status = system_get_state();

    //If the reset was done by a soft reset, it's not necessary to do the intro animation.
    if(status == SYS_SOFT_RESET){
        boot_screen_ani = false;
        system_set_state(SYS_NORMAL_STATE);
    }

    //The device can boot in 0.5 seconds, the magic of the micontrollers. But it think that it looks better
    // a fancy intro, and when it's showing the intro animation, all the peripherals are starting.
    xTaskCreatePinnedToCore(boot_screen_task, "intro_task", 2048, ( void * ) boot_screen_ani, 1, &intro_handler, 0);
    
    /**************** Message Queue initialization **************/

    //The firmware architecture was developed having in mind modularity, so we can now the system status by the message queue system
   
    batteryQueue = xQueueCreate(1, sizeof(struct BATTERY_STATUS));
    modeQueue = xQueueCreate(1, sizeof(struct SYSTEM_MODE));

    
    /**************** Peripherals initialization **************/
    //Initialize the peripherals
    audio_init(AUDIO_SAMPLE_16KHZ);
    sd_init();

    // The gamepad control and battery run on thread to avoid block on the code execution while it's running.
    input_init();
    battery_init();

    //If we are executing an update, and we reach this point, congratulations, the update was a succed!
    // Now we can disable the roll-back feature which is basically a bootloader tool which roll back to the previous
    //firmware if the new one doesn't work properly.
    update_check();

    //The boot animation is fancy, let's wait a little to see it
    if(boot_screen_ani) vTaskDelay(2000 / portTICK_RATE_MS);
    
    //Once we don't need the boot screen, we will delete the task and free the resources.
    vTaskDelete(intro_handler);
    boot_screen_free();

    /**************** GUI initialization **************/

    //This is embarrasing, is a temporary fix. The boot animation library use little endian logic and the rest of the firmware use big endian.
    //So, for now I'm not able to change it on the animation library. This function basically tell to the display to use big or little endian logic.
    display_HAL_change_endian();

    //Now we are ready to execute the GUI.
    xTaskCreatePinnedToCore(GUI_task, "Graphical User Interface", 1024*6, NULL, 1, &gui_handler, 0);
   

    bool game_running = false;
    bool game_executed = false;
    struct SYSTEM_MODE management;

    //Comments in progress

    while(1){
        
        if( xQueueReceive(modeQueue, &management ,portMAX_DELAY)){
            switch(management.mode){

                case MODE_GAME:
                    if(management.status == 1){//Execute the emulator.
                        battery_game_mode(true); //Block periodic battery status messages when your're playing.

                        //Check which console you've selected, execute the LED load animation and load the game.
                        if(management.console == GAMEBOY_COLOR || management.console == GAMEBOY){
                            LED_mode(LED_LOAD_ANI);
                            vTaskSuspend(gui_handler);
                            gnuboy_execute_game(management.game_name,management.console, management.load_save_game);
                            gnuboy_start();
                                
                            game_executed = true;
                            game_running=true;
                            console_running = management.console;
                            LED_mode(LED_TURN_OFF);
                            LED_mode(LED_FADE_ON);

                        }
                        else if(management.console == NES){
                            LED_mode(LED_LOAD_ANI);
                            vTaskSuspend(gui_handler);
                            NES_start(management.game_name);
                            //NES management it's slightly different so, it's necessary to first start the emulator.
                            if(management.load_save_game){
                                vTaskDelay(1500 / portTICK_RATE_MS);
                                NES_load_game();
                            }
                            game_executed = true;
                            game_running=true;
                            console_running = management.console;
                            LED_mode(LED_TURN_OFF);
                            LED_mode(LED_FADE_ON);
                        }
                        else if(management.console == SMS || management.console == GG){
                            LED_mode(LED_LOAD_ANI);
                            vTaskSuspend(gui_handler);
                            SMS_execute_game(management.game_name,management.console,management.load_save_game);
                            SMS_start();
                            game_executed = true;
                            game_running=true;
                            console_running = management.console;
                            LED_mode(LED_TURN_OFF);
                            LED_mode(LED_FADE_ON);
                        }
                    }
                    else{
                        if(game_running && game_executed){
                            //Suspend console main task to avoid conflicts.
                           if(console_running == GAMEBOY_COLOR || console_running == GAMEBOY ) gnuboy_suspend();
                           else if(console_running == NES) NES_suspend();
                           else if(console_running == SMS || console_running == GG) SMS_suspend();

                            // To avoid noise whe is suspend the audio task, is necesary to clean the dma from previous data.
                            audio_terminate();
                            vTaskResume(gui_handler);
                            GUI_refresh();
                            // Refresh menu image
                            game_running=false;
                        }
                        else if(!game_running && game_executed){ //This case is only used if we push menu button once you're on the on game menu
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
                    if(management.console == NES) NES_save_game();
                    else if(management.console == GAMEBOY_COLOR || management.console == GAMEBOY) gnuboy_save();
                    else if(management.console == SMS || management.console == GG) SMS_save_game();     
                break;

                case MODE_EXT_APP:
                    ESP_LOGI(TAG, "Loading external App");
                    vTaskSuspend(gui_handler);
                    LED_mode(LED_LOAD_ANI);
                    vTaskDelay(1000 / portTICK_RATE_MS);
                    external_app_init(management.game_name);
                    vTaskDelay(250 / portTICK_RATE_MS);
                    esp_restart();
                    vTaskDelay(250 / portTICK_RATE_MS);
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
                    system_set_state(SYS_SOFT_RESET);
                    ESP_LOGI(TAG,"System Soft Reset");
                    esp_restart();
                break;
            }
        }
        
        
    }
}