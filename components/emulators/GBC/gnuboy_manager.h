/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  gnuboy_start 
 * --------------------
 * 
 * Initialize the emulator core and the video/audio tasks.
 * 
 * NOTE: The game must be loaded previously.
 * 
 *  Returns: Nothing
 */
void gnuboy_start();

/*
 * Function:  gnuboy_resume 
 * --------------------
 * 
 * Pause the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void gnuboy_resume();

/*
 * Function:  gnuboy_suspend 
 * --------------------
 * 
 * Resume the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void gnuboy_suspend();

/*
 * Function:  gnuboy_save 
 * --------------------
 * 
 * Save the progress of the game into a file on the SD card.
 * 
 *  Returns: Nothing
 */
void gnuboy_save();

/*
 * Function:  gnuboy_load_game 
 * --------------------
 * 
 * Load the selected game from the SD card and if it exists the save game.
 * 
 * Arguments:
 * -name: Name of the game.
 * -console: Console to execute GAMEBOY_COLOR/GAMEBOY
 * 
 *  Returns: Nothing
 */
bool gnuboy_load_game(const char *name, uint8_t console);