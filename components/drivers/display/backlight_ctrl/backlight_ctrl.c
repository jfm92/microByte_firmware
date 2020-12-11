/*********************
 *      INCLUDES
 *********************/
#include "esp_log.h"
#include "driver/ledc.h"
#include "system_configuration.h"

#include "backlight_ctrl.h"

/**********************
*      VARIABLES
**********************/
ledc_channel_config_t backlight_led;
uint16_t backligth_level = 100;//Global variable to save the backlight value

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void backlight_init(){

    //Initialize LED timer
    ledc_timer_config_t ledc0_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT,
        .freq_hz = 8000,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_1,
        .clk_cfg = LEDC_AUTO_CLK,
    };

    ledc_timer_config(&ledc0_timer);

    //Configure the LEDC hardware
    backlight_led.channel = LEDC_CHANNEL_1;
    backlight_led.duty = 8000;
    backlight_led.gpio_num = HSPI_BACKLIGTH;
    backlight_led.speed_mode = LEDC_LOW_SPEED_MODE;
    backlight_led.hpoint = 0;
    backlight_led.timer_sel = LEDC_TIMER_1;

    ledc_channel_config(&backlight_led);

    ledc_fade_func_install(0);
}

void backlight_set(uint8_t level){
    
    if(level > 100) return;

    backligth_level = (8000 * level)/100; //Calcute the equivalent duty cycle

    //Set the change duty cycle to be done in 100 mS.
    ledc_set_fade_with_time(backlight_led.speed_mode, backlight_led.channel , backligth_level, 100);
    ledc_fade_start(backlight_led.speed_mode,backlight_led.channel , LEDC_FADE_NO_WAIT);
}

uint8_t backlight_get(){
    return backligth_level;
}