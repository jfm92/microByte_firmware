/*********************
*      INCLUDES
*********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/ledc.h"
#include "driver/rtc_io.h"
#include "esp_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "st7789.h"
#include "scr_spi.h"

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)


 /**********************
 *  STATIC PROTOTYPES
 **********************/

static void st7789_send_cmd(uint8_t cmd);
static void st7789_send_data(void *data, uint16_t length);
static void st7789_send_buffer(uint16_t *data, uint32_t data_size);
static void st7789_send_color(void * data, uint16_t length);

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




/**********************
 *   STATIC FUNCTIONS
 **********************/

static void st7789_send_cmd(uint8_t cmd){
	scr_spi_send(&cmd, 1, CMD_ON);
}

static void st7789_send_data(void *data, uint16_t length){
	scr_spi_send(data, length, DATA_ON);
}

static void st7789_send_color(void * data, uint16_t length)
{
   // scr_spi_transaction(data, length, 0);
}
