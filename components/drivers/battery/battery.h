/*
 * Function:  battery_init
 * --------------------
 * 
 * Initialize the ADC peripheral to get the battery level value
 * 
 *  Returns: Nothing
 */

void battery_init(void);

/*
 * Function:  battery_game_mode 
 * --------------------
 * 
 * Tell to the battery if we're on game mode to avoid send the periodical battery status.
 *  
 *  Arguments: 
 *      - game_mode: "true" to set the game mode otherwise "false"
 * 
 *  Returns: Nothing
 */

void battery_game_mode(bool game_mode);

/*
 * Function:  battery_get_percentage 
 * --------------------
 * 
 * Async battery get percentage level
 *  
 *  Returns: The actual level of the battery 0%->100%
 */


uint8_t battery_get_percentage();

/*
 * Function:  batteryTask 
 * --------------------
 * 
 * Task that controls every 1 sec the battery status.
 * Important! It shouldn't be call in any part of the code execpt on the main loop and starting as a task.
 *  
 *  Returns: Nothing
 */
void batteryTask(void *arg);