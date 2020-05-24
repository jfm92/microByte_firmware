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

uint16_t* line[LINE_BUFFERS];

static uint16_t getPixel(const uint16_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2);

void display_init(void){

    // Create display buffer
    const size_t lineSize = SCR_WIDTH * LINE_COUNT * sizeof(uint16_t);
    for (int x = 0; x < LINE_BUFFERS; x++){
        line[x] = heap_caps_malloc(lineSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
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

void display_image(){
    // TODO: Add support for display images
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
        uint16_t outHeight = SCR_HEIGHT;
        uint16_t outWidth = SCR_WIDTH;

        for(uint16_t y = 0; y < outHeight; y += LINE_COUNT){
            for(uint16_t i = 0; i < LINE_COUNT; i++){

                if((y + i) >= outHeight) break;

                uint16_t index = (i)*outWidth;

                for(uint16_t x = 0; x < outWidth; x++){
                    uint16_t sample = getPixel(data, x, (y + i), outWidth, outHeight);
                    
                    // Save the calculate pixel to the line buffer
                    index ++;
                    line[calc_line][index] = ((sample >> 8) | ((sample) << 8));
                }
            }

            //Print the calculated line
            sending_line = calc_line;
            calc_line = (calc_line == 1) ? 0 : 1;
            scr_spi_send_lines(y, 0, outWidth, line[sending_line], LINE_COUNT);
        }

    }
}

// REWORK
static uint16_t getPixel(const uint16_t *bufs, uint16_t x, uint16_t y, uint16_t w2, uint16_t h2){
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