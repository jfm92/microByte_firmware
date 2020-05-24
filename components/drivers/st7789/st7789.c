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

/*void st7789_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map)
{
    uint8_t data[4] = {0};

    //Column addresses
    st7789_send_cmd(ST7789_CASET);
    data[0] = (area->x1 >> 8) & 0xFF;
    data[1] = area->x1 & 0xFF;
    data[2] = (area->x2 >> 8) & 0xFF;
    data[3] = area->x2 & 0xFF;
    st7789_send_data(data, 4);

    //Page addresses
    st7789_send_cmd(ST7789_RASET);
    data[0] = (area->y1 >> 8) & 0xFF;
    data[1] = area->y1 & 0xFF;
    data[2] = (area->y2 >> 8) & 0xFF;
    data[3] = area->y2 & 0xFF;
    st7789_send_data(data, 4);

    //Memory write
    st7789_send_cmd(ST7789_RAMWR);

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

   // st7789_send_color((void*)color_map, size * 2);
   //TODO: Finish with menu work

}*/

/*void st7789_send_lines(uint8_t y_pos, uint8_t x_pos, uint8_t width, uint16_t *data, uint16_t frame_line){
    uint8_t data_buffer[4] = {0};
    uint8_t buffer_length = 0;
    
    // Column address set
    st7789_send_cmd(0x2A);
    data_buffer[0] = x_pos >> 8;
    data_buffer[1] = x_pos & 0xff;
    data_buffer[2] = (width + x_pos -1) >> 8;
    data_buffer[3] = (width + x_pos -1) & 0xFF;
    st7789_send_data(data_buffer,4);

    // Page address set
    st7789_send_cmd(0x2B);
    data_buffer[0] = y_pos >> 8;
    data_buffer[1] = y_pos & 0xff;
    data_buffer[2] = (y_pos + frame_line) >> 8;
    data_buffer[3] = (y_pos + frame_line) & 0xFF;
    st7789_send_data(data_buffer,4);

    st7789_send_cmd(0x2C);
    buffer_length = width * 2 * 8 * frame_line;
    st7789_send_buffer(data,buffer_length);

}*/


/**********************
 *   STATIC FUNCTIONS
 **********************/

static void st7789_send_cmd(uint8_t cmd){
	scr_spi_send(&cmd, 1, CMD_ON);
}

static void st7789_send_data(void *data, uint16_t length){
	scr_spi_send(data, length, DATA_ON);
}

/*static void st7789_send_buffer(uint16_t *data, uint32_t data_size){
    scr_spi_send_buffer(data, data_size);
}*/