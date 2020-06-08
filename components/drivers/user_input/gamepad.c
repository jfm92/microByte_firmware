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
    btn_config.pin_bit_mask = (uint64_t)      //Bitmask
                              ((uint64_t)1 << BTN_UP)   |
                              ((uint64_t)1 << BTN_DOWN) |
                              ((uint64_t)1 << BTN_RIGHT)|
                              ((uint64_t)1 << BTN_LEFT) |
                              ((uint64_t)1 << BTN_A)    |
                              ((uint64_t)1 << BTN_B)    |
                              ((uint64_t)1 << BTN_START)|
                              ((uint64_t)1 << BTN_SELECT);


    btn_config.pull_up_en = 0;      //Disable pullup
    btn_config.pull_down_en = 1; //Enable pulldown
    gpio_config(&btn_config);

}


uint8_t gamepad_state(uint8_t btn){

    return gpio_get_level(btn);
}

