#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"
/*********************
 *      DEFINES
 *********************/ 

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)

#define GB_FRAME_WIDTH  160 
#define GB_FRAME_HEIGHT 144 

#define NES_FRAME_WIDTH 256
#define NES_FRAME_HEIGHT 224

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void display_init(void);
void display_clear(void);
void display_gb_frame(const uint16_t *data);
void display_NES_frame(const uint8_t *data);
void display_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISPLAY_HAL_H*/