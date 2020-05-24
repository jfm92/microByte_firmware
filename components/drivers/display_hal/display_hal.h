#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/ 

#define LINE_BUFFERS (2)
#define LINE_COUNT   (4)

#define GB_FRAME_WIDTH 160
#define GB_FRAME_HEIGHT 144

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void display_init(void);
void display_clear(void);
void display_gb_frame(const uint16_t *data);


#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*DISPLAY_HAL_H*/