#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display_hal.h"
#include "scr_spi.h"
#include "st7789.h"
#include "system_configuration.h"

uint16_t *line[LINE_BUFFERS];

extern uint16_t myPalette[];

static uint16_t getPixelGBC(const uint16_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);
static uint8_t  getPixelNES(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);
static uint8_t  getPixelSms(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);

void display_init(void){

    // Create display buffer
    const size_t lineSize = SCR_WIDTH * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++){
       // line[x] = heap_caps_malloc(lineSize,  MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
        line[x] = heap_caps_malloc(lineSize,  MALLOC_CAP_8BIT | MALLOC_CAP_DMA );
        if (!line[x]) abort();
    }

    // Initialize SPI BUS
    scr_spi_init();

    // Initialize ST7789 screen
    st7789_init();
}

void display_clear(void){

    uint16_t sending_line=0;
    uint16_t calc_line=0;
    // clear the buffer
    for (uint16_t i = 0; i < LINE_BUFFERS; ++i){
        for (uint16_t j = 0; j < SCR_WIDTH * LINE_COUNT; ++j){
            line[i][j] = 0XFFFF; // White
        }
    }

    // Send each line with the value of the clean buffer
    for (uint16_t y = 0; y < SCR_HEIGHT; y += LINE_COUNT){
        sending_line=calc_line;
        calc_line=(calc_line==1)?0:1;
        scr_spi_send_lines(y, 0, SCR_WIDTH, line[sending_line], LINE_COUNT);
    }
}

void display_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map){
    st7789_flush(drv,area,color_map);
}

void display_gb_frame(const uint16_t *data){

    uint16_t calc_line = 0;
    uint16_t sending_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                line[calc_line][x] = 0; 
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, 0, SCR_WIDTH, line[sending_line], 1);
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
                    line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                }
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }    
    }
}

void display_NES_frame(const uint8_t *data){
    uint16_t calc_line = 0;
    uint16_t sending_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                line[calc_line][x] = 0; 
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, 0, SCR_WIDTH, line[sending_line], 1);
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
                    line[calc_line][index++] = myPalette[getPixelNES(data, x, (y + i), outputWidth, outputHeight)];
                }
            }

            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }
    }


}

void display_SMS_frame(const uint8_t *data, uint16_t color[]){
    uint16_t sending_line = 0;
    uint16_t calc_line = 0;

    if(data == NULL){
        for(uint16_t y = 0; y < SCR_HEIGHT; y++){

            for(uint16_t x = 0; x < SCR_WIDTH; x++){
                line[calc_line][x] = 0; 
            }
            
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, 0, SCR_WIDTH, line[sending_line], 1);
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
                    uint16_t sample = color[getPixelSms(data, x, (y + i), outputWidth, outputHeight) & PIXEL_MASK];
                    line[calc_line][index++] = ((sample >> 8) | ((sample) << 8));
                    //line[calc_line][index++] = color[getPixelSms(data, x, (y + i), outputWidth, outputHeight)];
                }
            }
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, xpos, outputWidth, line[sending_line], LINE_COUNT);
        }
    }

}

static uint8_t getPixelSms(const uint8_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2){
    int x_diff, y_diff, xv, yv, red, green, blue, col, a, b, c, d, index;
    int x_ratio = (int)(((SMS_FRAME_WIDTH - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((SMS_FRAME_HEIGHT - 1) << 16) / h2) + 1;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    /*if (isGameGear)
    {
        index = yv * 256 + xv + 48;
    }
    else
    {*/
    index = yv * SMS_FRAME_WIDTH + xv;
    //}

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + SMS_FRAME_WIDTH];
    d = bufs[index + SMS_FRAME_WIDTH + 1];

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
    int x_ratio = (int)(((GB_FRAME_WIDTH - 1) << 16) / w2) + 1;
    int y_ratio = (int)(((GB_FRAME_HEIGHT - 1) << 16) / h2) + 1;

    xv = (int)((x_ratio * x) >> 16);
    yv = (int)((y_ratio * y) >> 16);

    x_diff = ((x_ratio * x) >> 16) - (xv);
    y_diff = ((y_ratio * y) >> 16) - (yv);

    index = yv * GB_FRAME_WIDTH + xv;

    a = bufs[index];
    b = bufs[index + 1];
    c = bufs[index + GB_FRAME_WIDTH];
    d = bufs[index + GB_FRAME_WIDTH + 1];

    red = (((a >> 11) & 0x1f) * (1 - x_diff) * (1 - y_diff) + ((b >> 11) & 0x1f) * (x_diff) * (1 - y_diff) +
           ((c >> 11) & 0x1f) * (y_diff) * (1 - x_diff) + ((d >> 11) & 0x1f) * (x_diff * y_diff));

    green = (((a >> 5) & 0x3f) * (1 - x_diff) * (1 - y_diff) + ((b >> 5) & 0x3f) * (x_diff) * (1 - y_diff) +
             ((c >> 5) & 0x3f) * (y_diff) * (1 - x_diff) + ((d >> 5) & 0x3f) * (x_diff * y_diff));

    blue = (((a)&0x1f) * (1 - x_diff) * (1 - y_diff) + ((b)&0x1f) * (x_diff) * (1 - y_diff) +
            ((c)&0x1f) * (y_diff) * (1 - x_diff) + ((d)&0x1f) * (x_diff * y_diff));

    col = ((int)red << 11) | ((int)green << 5) | ((int)blue);

    return col;
}


