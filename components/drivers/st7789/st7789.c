/*********************
*      INCLUDES
*********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "driver/rtc_io.h"
#include "esp_system.h"

#include "driver/ledc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "st7789.h"
#include "scr_spi.h"
#include "system_configuration.h"

/**********************
 *      DEFINE
 **********************/

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)

/**********************
 *  STATIC VARIABLES
 **********************/

static ledc_channel_config_t backlight_channel;

/**********************
 *      VARIABLES
 **********************/
uint32_t backlight_level = 80; // 80% of the brightness

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void st7789_send_cmd(uint8_t cmd);
static void st7789_send_data(void *data, uint16_t length);
static void st7789_send_buffer(uint16_t *data, uint32_t data_size);
static void st7789_backlight_init();

 /**********************
 *   GLOBAL FUNCTIONS
 **********************/

void st7789_init(void){

    // Initalization sequence
    lcd_init_cmd_t st7789_init_cmds[] = {
        {0xCF, {0x00, 0x83, 0X30}, 3},
        {0xED, {0x64, 0x03, 0X12, 0X81}, 4},
        {ST7789_PWCTRL2, {0x85, 0x01, 0x79}, 3},
        {0xCB, {0x39, 0x2C, 0x00, 0x34, 0x02}, 5},
        {0xF7, {0x20}, 1},
        {0xEA, {0x00, 0x00}, 2},
        {ST7789_LCMCTRL, {0x26}, 1},
        {ST7789_IDSET, {0x11}, 1},
        {ST7789_VCMOFSET, {0x35, 0x3E}, 2},
        {ST7789_CABCCTRL, {0xBE}, 1},
        {ST7789_MADCTL, {0x00}, 1}, // Set to 0x28 if your display is flipped
        {ST7789_COLMOD, {0x55}, 1},
	{ST7789_INVON, {0}, 0},
        {ST7789_RGBCTRL, {0x00, 0x1B}, 2},
        {0xF2, {0x08}, 1},
        {ST7789_GAMSET, {0x01}, 1},
        {ST7789_PVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0E, 0x12, 0x14, 0x17}, 14},
        {ST7789_NVGAMCTRL, {0xD0, 0x00, 0x02, 0x07, 0x0A, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
        {ST7789_CASET, {0x00, 0x00, 0x00, 0xEF}, 4},
        {ST7789_RASET, {0x00, 0x00, 0x01, 0x3f}, 4},
        {ST7789_RAMWR, {0}, 0},
        {ST7789_GCTRL, {0x07}, 1},
        {0xB6, {0x0A, 0x82, 0x27, 0x00}, 4},
        {ST7789_SLPOUT, {0}, 0},
        {ST7789_DISPON, {0}, 0},
        {0, {0}, 0xff},
    };

    // Reset the screen
    scr_spi_rst();

    printf("ST7789 initialization \n");

    uint16_t cmd = 0;
    while (st7789_init_cmds[cmd].databytes != 0xff)
	{
		st7789_send_cmd(st7789_init_cmds[cmd].cmd);
		st7789_send_data(st7789_init_cmds[cmd].data, st7789_init_cmds[cmd].databytes & 0x1F);
		if (st7789_init_cmds[cmd].databytes & 0x80)
		{
			vTaskDelay(100 / portTICK_RATE_MS);
		}
		cmd++;
	}

    st7789_backlight_init();
    st7789_backlight_set(backlight_level);

}


void st7789_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    uint8_t data[4] = {0};

    uint16_t offsetx1 = area->x1;
    uint16_t offsetx2 = area->x2;
    uint16_t offsety1 = area->y1;
    uint16_t offsety2 = area->y2;

      /*Column addresses*/
    st7789_send_cmd(ST7789_CASET);
    data[0] = (offsetx1 >> 8) & 0xFF;
    data[1] = offsetx1 & 0xFF;
    data[2] = (offsetx2 >> 8) & 0xFF;
    data[3] = offsetx2 & 0xFF;
    st7789_send_data(data, 4);

    /*Page addresses*/
    st7789_send_cmd(ST7789_RASET);
    data[0] = (offsety1 >> 8) & 0xFF;
    data[1] = offsety1 & 0xFF;
    data[2] = (offsety2 >> 8) & 0xFF;
    data[3] = offsety2 & 0xFF;
    st7789_send_data(data, 4);

    /*Memory write*/
    st7789_send_cmd(ST7789_RAMWR);

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    st7789_send_data((void *)color_map, size * 2);
 
    lv_disp_flush_ready(drv);
    
}

void st7789_backlight_set(uint8_t level){
    backlight_level = ((level * 5000)/100);

    ledc_set_fade_with_time(backlight_channel.speed_mode,
                    backlight_channel.channel, backlight_level, 1000);
    ledc_fade_start(backlight_channel.speed_mode,
            backlight_channel.channel, LEDC_FADE_NO_WAIT);
    
}

uint8_t st7789_backlight_get(){
    return backlight_level;
}




/**********************
 *   STATIC FUNCTIONS
 **********************/

static void st7789_send_cmd(uint8_t cmd){
	scr_spi_send(&cmd, 1, CMD_ON);
}

static void st7789_send_data(void *data, uint16_t length){
	scr_spi_send(data, length, DATA_ON);
}

static void st7789_backlight_init(){

    ledc_timer_config_t backligth_timer = {
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };

    ledc_timer_config(&backligth_timer);

    backlight_channel.channel   = LEDC_CHANNEL_0;
    backlight_channel.duty      = 100;
    backlight_channel.gpio_num  = HSPI_BACKLIGTH;
    backlight_channel.hpoint    = 0;
    backlight_channel.timer_sel = LEDC_TIMER_0;

    ledc_channel_config(&backlight_channel);

    ledc_fade_func_install(0);
}