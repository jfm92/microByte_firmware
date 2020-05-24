#include <stdio.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "system_configuration.h"
#include "gamepad.h"

gpio_config_t btn_config;

void gamepad_init(){

        
    btn_config.intr_type = GPIO_INTR_ANYEDGE; //Enable interrupt on both rising and falling edges
    btn_config.mode = GPIO_MODE_INPUT;        //Set as Input
    btn_config.pin_bit_mask = ((1ULL<<BTN_UP) | (1ULL<<BTN_DOWN)) ;


    btn_config.pull_up_en = 0;      //Disable pullup
    btn_config.pull_down_en = 1; //Enable pulldown
    gpio_config(&btn_config);

}

void gamepad_task(void * arg){
    gamepad_init();
    
    while(1){
        uint8_t level = 0;
        level = gpio_get_level(BTN_UP);
        if(level == 1) printf("btn\r\n");
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

