/*********************
*      INCLUDES
*********************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "TCA9555.h"
//#include "st7789.h"
#include "sound_driver.h"
#include "user_input.h"

#include "system_configuration.h"
#include "system_manager.h"

/**********************
 *      VARIABLES
 **********************/

uint32_t menu_btn_time =0;


/**********************
 *      STATIC 
 **********************/
static const char *TAG = "user_input";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void input_init(void){
    // Initalize mux driver
    ESP_LOGI(TAG,"Initalization of GPIO mux driver");
    TCA955_init();
}

//TODO: - Volume rapid change issue, it only goes up to 53 %.
//      - Brightness issue, the brightness value jumps probrably an issue with the fade function


uint16_t input_read(void){

    //Get the mux values
    uint16_t inputs_value = TCA9555_readInputs();

    //Check if the menu button it was pushed
    if(!((inputs_value >>11) & 0x01)){ //Temporary workaround !((inputs_value >>11) & 0x01) is the real button

            struct SYSTEM_MODE management;

            //Get the actual time
            uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

            // Check if any of the special buttons was pushed
            if(!((inputs_value >> 0) & 0x01)){
                // Down arrow, volume down
                int volume_aux = audio_volume_get();
                volume_aux -= 10;
                if(volume_aux < 0)volume_aux = 0;
                
                management.mode = MODE_CHANGE_VOLUME;
                management.volume_level = volume_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");

            } 
            else if(!((inputs_value >> 2) & 0x01)){
                //UP arrow, volume UP
                int volume_aux = audio_volume_get();
                volume_aux += 10;
                if(volume_aux > 100)volume_aux = 100;
                
                management.mode = MODE_CHANGE_VOLUME;
                management.volume_level = volume_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else if(!((inputs_value >> 1) & 0x01)){
                // Right arrow, brightness up
                int brightness_aux = 0;//st7789_backlight_get();
                brightness_aux += 10;
                if(brightness_aux > 100)brightness_aux = 100;
                
                management.mode = MODE_CHANGE_BRIGHT;
                management.volume_level = brightness_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else if(!((inputs_value >> 3) & 0x01)){
                // Left arrow, brightness down
                int brightness_aux = 0;//st7789_backlight_get();
                brightness_aux -= 10;
                if(brightness_aux < 0 )brightness_aux = 0;
                
                management.mode = MODE_CHANGE_BRIGHT;
                management.volume_level = brightness_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else{
                if((actual_time-menu_btn_time)>25){
                    printf("Menu\r\n");
                    management.mode = MODE_GAME;
                    management.status = 0;

                    if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
                    menu_btn_time = actual_time;
                }
                
            }
            return 0xFFFF;
    }
    else{
        return inputs_value;
    }
}




