#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"

#include "system_configuration.h"
#include "LED_notification.h"

QueueHandle_t LEDQueue;

static void LED_task(void *arg);

void LED_init(){
    gpio_pad_select_gpio(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 1);

    LEDQueue = xQueueCreate(5, sizeof(uint8_t));

    xTaskCreatePinnedToCore(&LED_task, "Notification LED task", 1024, NULL, 5, NULL, 0);

}

void LED_mode(uint8_t mode){

}

static void LED_task(void *arg){
    uint8_t mode;

    while(1){

        if(xQueueReceive(LEDQueue, &mode ,portMAX_DELAY)){
            //if(mode == )
        }
    }
}