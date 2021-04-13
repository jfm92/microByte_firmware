/*********************
 *      INCLUDES
 *********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ST7789_driver.h"
#include "display_HAL.h"
#include "system_configuration.h"

/*********************
 *      DEFINES
 *********************/
#define LINE_BUFFERS (2)
#define LINE_COUNT   (20)

#define GBC_FRAME_WIDTH  160 
#define GBC_FRAME_HEIGHT 144

#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 240

#define SMS_FRAME_WIDTH 256
#define SMS_FRAME_HEIGHT 192

#define GG_FRAME_WIDTH 160
#define GG_FRAME_HEIGHT 144

#define PIXEL_MASK (0x1F) 

uint16_t *line[LINE_BUFFERS];

extern uint16_t myPalette[];

/**********************
*      VARIABLES
**********************/
st7789_driver_t display = {
		.pin_reset = HSPI_RST,
		.pin_dc = HSPI_DC,
		.pin_mosi = HSPI_MOSI,
		.pin_sclk = HSPI_CLK,
		.spi_host = HSPI_HOST,
		.dma_chan = 1,
		.display_width = 240,
		.display_height = 240,
		.buffer_size = 20 * 240, // 2 buffers with 20 lines
	};

static const char *TAG = "Display_HAL";

/**********************
*  STATIC PROTOTYPES
**********************/
static uint16_t getPixelGBC(const uint16_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);
static uint8_t getPixelSMS(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2, bool GAME_GEAR);
static uint8_t getPixelNES(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

// Display HAL basic functions.

bool display_HAL_init(void){
    return ST7789_init(&display);
}

void display_HAL_clear(){
    ST7789_fill_area(&display, WHITE, 0, 0, display.display_width, display.display_height);
}

// Boot Screen Functions
uint16_t * display_HAL_get_buffer(){
    return display.current_buffer;
}

size_t display_HAL_get_buffer_size(){
    return display.buffer_size;
}

void display_HAL_boot_frame(uint16_t * buffer){
    // The boot animation to the buffer
    display.current_buffer = buffer;

    //Send to the driver layer and change the buffer
    ST7789_swap_buffers(&display);
}

void display_HAL_change_endian(){
    ST7789_set_endian(&display);
}

// LVGL library releated functions
void display_HAL_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map){

    uint32_t size = lv_area_get_width(area) * lv_area_get_height(area);

    //Set the area to print on the screen
    ST7789_set_window(&display,area->x1,area->y1,area->x2 ,area->y2);

    //Save the buffer data and the size of the data to send
    display.current_buffer = (void *)color_map;
    display.buffer_size = size;

    //Send it
    //ST7789_write_pixels(&display, display.current_buffer, display.buffer_size);
    ST7789_swap_buffers(&display);

    //Tell to LVGL that is ready to send another frame
    lv_disp_flush_ready(drv);
}

// Emulators frame generation functions.
void display_HAL_gb_frame(const uint16_t *data){
    uint16_t calc_line = 0;
    uint16_t sending_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                display.current_buffer[x] = 0;
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            ST7789_write_lines(&display,y, 0, SCR_WIDTH, line[sending_line], 1);
        }
    }
    else{
        short outputHeight = SCR_HEIGHT;
        short outputWidth = 240;
        short xpos = (SCR_WIDTH - outputWidth) / 2;

        for (int y = 0; y < outputHeight; y += LINE_COUNT)
        {
            for (int i = 0; i < LINE_COUNT; ++i)
            {
                if ((y + i) >= outputHeight)
                    break;

                int index = (i)*outputWidth;

                for (int x = 0; x < outputWidth; ++x)
                {
                    uint16_t sample = getPixelGBC(data, x, (y + i), outputWidth, outputHeight);
                   display.current_buffer[index++] = ((sample >> 8) | ((sample) << 8));
                }
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
           // ST7789_swap_buffers(&display);
            ST7789_write_lines(&display,y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }  
    }
}

void display_HAL_NES_frame(const uint8_t *data){
    uint16_t calc_line = 0;
    uint16_t sending_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                display.current_buffer[x] = 0;
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            ST7789_write_lines(&display,y, 0, SCR_WIDTH, line[sending_line], 1);
        }
    }
    else{
        short outputHeight = 240;
        short outputWidth = 240 + (240 - 240);
        short xpos = (240 - outputWidth) / 2;

        for (int y = 0; y < outputHeight; y += LINE_COUNT){
            for (int i = 0; i < LINE_COUNT; ++i){
                if ((y + i) >= outputHeight)
                break;

                int index = (i)*outputWidth;

                for (int x = 0; x < outputWidth; x++){
                    display.current_buffer[index++]= myPalette[getPixelNES(data, x, (y + i), outputWidth, outputHeight)];
                }
            }

            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            ST7789_write_lines(&display,y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }
    }
}

void display_HAL_SMS_frame(const uint8_t *data, uint16_t color[], bool GAMEGEAR){
    uint16_t sending_line = 0;
    uint16_t calc_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                display.current_buffer[x] = 0;
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            ST7789_write_lines(&display,y, 0, SCR_WIDTH, line[sending_line], 1);
        }
    }
    else{
        short outputHeight = SCR_HEIGHT;
        short outputWidth = SCR_WIDTH;
        short xpos = (SCR_WIDTH - outputWidth) / 2;

        for (int y = 0; y < outputHeight; y += LINE_COUNT){
            for (int i = 0; i < LINE_COUNT; ++i){
                if ((y + i) >= outputHeight)
                    break;

                int index = (i)*outputWidth;

                for (int x = 0; x < outputWidth; ++x){
                    //uint16_t sample = color[getPixelSms(data, x, (y + i), outputWidth, outputHeight)];
                    uint16_t sample = color[getPixelSMS(data, x, (y + i), outputWidth, outputHeight, GAMEGEAR) & PIXEL_MASK];
                    display.current_buffer[index++] = ((sample >> 8) | ((sample) << 8));
                    //line[calc_line][index++] = color[getPixelSms(data, x, (y + i), outputWidth, outputHeight)];
                }
            }
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            ST7789_write_lines(&display,y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }
    }

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static uint8_t getPixelSMS(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2, bool GAME_GEAR){
    uint16_t frame_width = SMS_FRAME_WIDTH;
    uint16_t frame_height = SMS_FRAME_HEIGHT;
    if(GAME_GEAR){
        frame_width = GG_FRAME_WIDTH;
        frame_height = GG_FRAME_HEIGHT;
    }

    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;
    int x_ratio = (int)((((frame_width) - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((frame_height - 1) << 16) / h2) + 1;


    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    if (GAME_GEAR)
    {
        index = yv * 256 + xv + 48;
    }
    else
    {
    index = yv * frame_width + xv;
    }

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + frame_width];
    d = bufs[index + frame_width + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}

static uint8_t getPixelNES(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2){

    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;
    int x_ratio = (int)(((NES_FRAME_WIDTH - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((NES_FRAME_HEIGHT - 1) << 16) / h2) + 1;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    index = yv * NES_FRAME_WIDTH + xv;

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + NES_FRAME_WIDTH];
    d = bufs[index + NES_FRAME_WIDTH + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}

static uint16_t getPixelGBC(const uint16_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2){

    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;
    int x_ratio = (int)(((GBC_FRAME_WIDTH - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((GBC_FRAME_HEIGHT - 1) << 16) / h2) + 1;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    index = yv * GBC_FRAME_WIDTH + xv;

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + GBC_FRAME_WIDTH];
    d = bufs[index + GBC_FRAME_WIDTH + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}