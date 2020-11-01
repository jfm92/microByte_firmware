/*********************
*      INCLUDES
*********************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"

#include "driver/gpio.h"

#include "TCA9555.h"
#include "st7789.h"
#include "user_input.h"
#include "system_configuration.h"
#include "sound_driver.h"
#include "system_manager.h"

/**********************
 *      VARIABLES
 **********************/

uint32_t menu_btn_time =0;
static const char *TAG = "user_input";




 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

/*
 * Function:  input_init 
 * --------------------
 * 
 * Initialize the mux driver and set the right configuration.
 * 
 *  Returns: Nothing
 */

void input_init(void){
    // Initalize mux driver
    ESP_LOGI(TAG,"Initalization of GPIO mux driver");
    TCA955_init();

    // Configure mux interruption -> Deprecrated
    /*gpio_config_t input_config;

    input_config.intr_type = GPIO_INTR_NEGEDGE;
    input_config.pin_bit_mask = GPIO_PIN_SEL;
    input_config.mode = GPIO_MODE_INPUT;
    input_config.pull_up_en = 0;
    input_config.pull_down_en = 0;
    gpio_config(&input_config);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(MUX_INT, input_handler, (void*) MUX_INT);*/

    //When GPIO change the input status in the mux, the TCA9555 rise an interruption on the MCU
    //to advertise that somenthing has change.

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
                volume_aux -= 1;
                if(volume_aux < 0)volume_aux = 0;
                
                management.mode = MODE_CHANGE_VOLUME;
                management.volume_level = volume_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");

            } 
            else if(!((inputs_value >> 2) & 0x01)){
                //UP arrow, volume UP
                int volume_aux = audio_volume_get();
                volume_aux += 1;
                if(volume_aux > 100)volume_aux = 100;
                
                management.mode = MODE_CHANGE_VOLUME;
                management.volume_level = volume_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else if(!((inputs_value >> 1) & 0x01)){
                // Right arrow, brightness up
                int brightness_aux = st7789_backlight_get();
                brightness_aux += 10;
                if(brightness_aux > 100)brightness_aux = 100;
                
                management.mode = MODE_CHANGE_BRIGHT;
                management.volume_level = brightness_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else if(!((inputs_value >> 3) & 0x01)){
                // Left arrow, brightness down
                int brightness_aux = st7789_backlight_get();
                brightness_aux -= 10;
                if(brightness_aux < 0 )brightness_aux = 0;
                
                management.mode = MODE_CHANGE_BRIGHT;
                management.volume_level = brightness_aux;
                
                if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
            }
            else{
                if((actual_time-menu_btn_time)>25){

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

/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Deprecrated
#define GPIO_PIN_SEL  ((1ULL<<MUX_INT))

#define ESP_INTR_FLAG_DEFAULT 0
static void input_read_task(void *arg);


static void input_handler (void* arg){
    
    uint8_t input_status = 1; // This is very ugly
    xQueueSendFromISR(input_queue, &input_status, NULL);
}
*/



