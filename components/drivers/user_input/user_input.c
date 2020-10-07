
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "driver/gpio.h"

#include "TCA9555.h"
#include "user_input.h"
#include "system_configuration.h"

#define GPIO_PIN_SEL  ((1ULL<<MUX_INT))

#define ESP_INTR_FLAG_DEFAULT 0


static void input_handler (void* arg){
    
    uint8_t input_status = 1; // This is very ugly
    xQueueSendFromISR(input_queue, &input_status, NULL);
}


void input_init(void){
    // Initalize mux driver
    TCA955_init();

    // Configure mux interruption
    gpio_config_t input_config;

    input_config.intr_type = GPIO_INTR_NEGEDGE;
    input_config.pin_bit_mask = GPIO_PIN_SEL;
    input_config.mode = GPIO_MODE_INPUT;
    input_config.pull_up_en = 0;
    input_config.pull_down_en = 0;
    gpio_config(&input_config);

    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(MUX_INT, input_handler, (void*) MUX_INT);

    //When GPIO change the input status in the mux, the TCA9555 rise an interruption on the MCU
    //to advertise that somenthing has change.

}

uint16_t input_read(void){
/**
 * Bit position of each button
 * - 0 -> Down     - 2 -> Up
 * - 1 -> Left     - 3 -> Right
 * 
 * - 12 -> Select  - 11 -> Menu
 * - 10 -> Start
 * 
 * - 9 -> B    - 8 -> A    - 7 -> Y
 * - 6 -> X    - 5 -> R    - 13 -> L
 * */

    return TCA9555_readInputs();
}



