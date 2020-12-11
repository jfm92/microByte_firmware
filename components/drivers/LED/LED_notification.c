/*********************
 *      INCLUDES
 *********************/
#include <stdio.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "driver/ledc.h"

#include "system_configuration.h"
#include "LED_notification.h"

/**********************
*      VARIABLES
**********************/
QueueHandle_t LEDQueue;
ledc_channel_config_t ledc;

/**********************
*  STATIC PROTOTYPES
**********************/
static void LED_task(void *arg);
static const char *TAG = "LED notifications";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void LED_init(){

    //Initialize LED timer
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 5000,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&ledc_timer);

    //Configure the LEDC hardware
    ledc.channel = LEDC_CHANNEL_0;
    ledc.duty = 0;
    ledc.gpio_num = LED_PIN;
    ledc.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc.hpoint = 0;
    ledc.timer_sel = LEDC_TIMER_0;

    ledc_channel_config(&ledc);

    ledc_fade_func_install(0);

    //Queue to send message to the LED control task
    LEDQueue = xQueueCreate(5, sizeof(uint8_t));

    //To avoid blocking operations is better to set the LED control on a task
    xTaskCreatePinnedToCore(&LED_task, "Notification LED task", 1024, NULL, 5, NULL, 0);
}

void LED_mode(uint8_t mode){
    if( xQueueSend( LEDQueue,&mode, ( TickType_t ) 10) != pdPASS ) ESP_LOGE(TAG, "Queue send failed");
}


/**********************
 *   STATIC FUNCTIONS
 **********************/
static void LED_task(void *arg){
    uint8_t mode;

    while(1){
        if( xQueueReceive(LEDQueue, &mode ,portMAX_DELAY) || mode != NULL){
            if(mode == LED_TURN_ON){
                //Just turn on the LED with a fast fade animation.
                ledc_set_fade_with_time(ledc.speed_mode,
                ledc.channel , 5000, 100);
                ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                mode = NULL;
            }
            else if(mode == LED_TURN_OFF){
                //Just turn off the LED with a fast fade animation.
                ledc_set_fade_with_time(ledc.speed_mode,
                ledc.channel , 0, 100);
                ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                mode = NULL;
            }
            else if(mode == LED_BLINK_HS){

                while(1){
                    if(xQueueReceive(LEDQueue, &mode ,0)) break;//If we receive a new message, break the loop.

                    //Fade max level and 0 every 250 mS
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 5000, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                    
                    vTaskDelay(250 / portTICK_RATE_MS);
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 0, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                    
                    vTaskDelay(250 / portTICK_RATE_MS);
                }

            }
            else if(mode == LED_BLINK_LS){
                while(1){
                    if(xQueueReceive(LEDQueue, &mode ,0)) break;//If we receive a new message, break the loop.

                    //Fade max level and 0 every 1 S
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 5000, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                    
                    vTaskDelay(1000 / portTICK_RATE_MS);
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 0, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                    
                    vTaskDelay(1000 / portTICK_RATE_MS);
                }

            }
            else if(mode == LED_FADE_ON){
                ledc_set_fade_with_time(ledc.speed_mode,
                ledc.channel , 5000, 1000);
                ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                mode = NULL;
            }
            else if(mode == LED_FADE_OFF){
                ledc_set_fade_with_time(ledc.speed_mode,
                ledc.channel , 0, 1000);
                ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                mode = NULL;
            }
            else if(mode == LED_LOAD_ANI){
                while(1){
                    if(xQueueReceive(LEDQueue, &mode ,0)) break;//If we receive a new message, break the loop.
                    int random_time = rand() % 250;
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 5000, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);
                    
                    vTaskDelay(random_time / portTICK_RATE_MS);
                    ledc_set_fade_with_time(ledc.speed_mode,
                    ledc.channel , 0, 100);
                    ledc_fade_start(ledc.speed_mode,ledc.channel , LEDC_FADE_NO_WAIT);

                    random_time = rand() % 250;
                    vTaskDelay(random_time / portTICK_RATE_MS);
                }
            }

        }
    }
}