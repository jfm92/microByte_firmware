#include <freertos/queue.h>

/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  NES_start 
 * --------------------
 * 
 * Initialize the emulator core and the video/audio tasks.
 * 
 * NOTE: The game must be loaded previously.
 * 
 *  Returns: Nothing
 */
void NES_start(const char *game_name);

/*
 * Function:  NES_resume 
 * --------------------
 * 
 * Pause the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void NES_resume();

/*
 * Function:  NES_suspend 
 * --------------------
 * 
 * Resume the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void NES_suspend();

/*
 * Function:  NES_load_game 
 * --------------------
 * 
 * Load the selected game from the SD card and if it exists the save game.
 * 
 * Arguments:
 * -name: Name of the game.
 * 
 *  Returns: Nothing
 */
void NES_load_game();

/*
 * Function:  NES_save_game 
 * --------------------
 * 
 * Save the progress of the game into a file on the SD card.
 * 
 *  Returns: Nothing
 */
void NES_save_game();

QueueHandle_t nofrendo_vidQueue;
QueueHandle_t nofrendo_audioQueue;
