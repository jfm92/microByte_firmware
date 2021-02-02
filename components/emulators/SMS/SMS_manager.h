/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  SMS_start 
 * --------------------
 * 
 * Initialize the emulator core and the video/audio tasks.
 * 
 * NOTE: The game must be loaded previously.
 * 
 *  Returns: Nothing
 */
void SMS_start();

/*
 * Function:  SMS_resume 
 * --------------------
 * 
 * Pause the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void SMS_resume();

/*
 * Function:  SMS_suspend 
 * --------------------
 * 
 * Resume the execution of the emulator and the video/audio tasks.
 * 
 *  Returns: Nothing
 */
void SMS_suspend();


/*
 * Function:  SMS_execute_game 
 * --------------------
 * 
 * Load the selected game from the SD card and load it into the RAM memory
 * 
 * Arguments:
 * -name: Name of the game.
 * -console: Console to execute SMS(Sega Master System)/GG(Game Gear)
 * -load: If you want to load previous save game data.
 * 
 *  Returns: True if it was loaded the selected game, otherwise false.
 */
bool SMS_execute_game(const char *game_name, uint8_t console, bool load);

/*
 * Function:  SMS_save_game 
 * --------------------
 * 
 * Save the progress of the game into a file on the SD card.
 * 
 *  Returns: Nothing
 */
void SMS_save_game();