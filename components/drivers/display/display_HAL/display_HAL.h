/*********************
 *      INCLUDES
 *********************/
#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define BLACK 0x0000
#define WHITE 0xFFFF

/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  display_HAL_init 
 * --------------------
 * 
 * Initialize the screen
 * 
 * Returns: True if the initialization suceed otherwise false.
 * 
 */
bool display_HAL_init(void);

/*
 * Function:  display_HAL_clear 
 * --------------------
 * 
 * Fill the entire screen in white color.
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_clear();

/*
 * Function:  display_HAL_flush 
 * --------------------
 * 
 * LVGL library function which flush a partial frame of 20 px x 240 px
 * 
 * Arguments:
 *  - lv_disp_drv_t * drv: Instance to the screen defined on the LVGL initialization.
 *  - const lv_area_t * area: Screen area information which will be updated.
 *  - lv_color_t * color_map: Color map of the area to be update.
 * Returns: Nothing
 * 
 * Note: This function should not be called from any other part of the code, because
 * is called periodically by the LVGL library.
 * 
 */
void display_HAL_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_map);

/*
 * Function:  display_HAL_gb_frame 
 * --------------------
 * 
 * Process the emulator information to set the color and scale of the frame, and send the
 * information to the screen driver.
 * 
 * Arguments:
 *  - data: Frame data of the GameBoy/GameBoy color emulator.
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_gb_frame(const uint16_t *data);

/*
 * Function:  display_HAL_NES_frame 
 * --------------------
 * 
 * Process the emulator information to set the color and scale of the frame, and send the
 * information to the screen driver.
 * 
 * Arguments:
 *  - data: Frame data of the NES color emulator.
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_NES_frame(const uint8_t *data);

/*
 * Function:  display_HAL_SMS_frame 
 * --------------------
 * 
 * Process the emulator information to set the color and scale of the frame, and send the
 * information to the screen driver.
 * 
 * Arguments:
 *  - data: Frame data of the Sega Master System or Game Gear emulator, without color.
 *  - color: Color to apply to each frame.
 *  - GAMEGEAR: Set to true if you want to create a Game Gear frame, otherwise for Master System set to false
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_SMS_frame(const uint8_t *data, uint16_t color[], bool GAMEGEAR);

/*
 * Function:  display_HAL_get_buffer 
 * --------------------
 * 
 * This function return a pointer to use the current display buffer.
 * 
 * Returns: Pointer to the current buffer in use.
 * 
 */
uint16_t * display_HAL_get_buffer();

/*
 * Function:  display_HAL_get_buffer_size 
 * --------------------
 * 
 * Give the actual size of the buffer.
 * 
 * Returns: Size of the buffer
 * 
 */
size_t display_HAL_get_buffer_size();

/*
 * Function:  display_HAL_boot_frame 
 * --------------------
 * 
 * This function receive the pointer of a buffer with the partial boot frame to print
 * on the screen and send it to the driver layer.
 * 
 * Arguments:
 *  -buffer: Pointer rendered buffer.
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_boot_frame(uint16_t * buffer);

/*
 * Function:  display_HAL_change_endian 
 * --------------------
 * 
 * This function is a workaround to solve the issue with little endian and big endian package.
 * The boot animation library creates little endian package on the other hand the GUI and the emulators,
 * use big endian.
 * By default the screen is configured to use little endian, but is necessay to change the endian configuration
 * to obtain proper image after the boot screen.
 * 
 * 
 * Returns: Nothing
 * 
 */
void display_HAL_change_endian();