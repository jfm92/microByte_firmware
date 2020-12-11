/*
 * Function:  backlight_init 
 * --------------------
 * 
 * Initialize the display backlight control
 * 
 * 
 * Returns: Nothing
 * 
 */
void backlight_init();

/*
 * Function:  backlight_set 
 * --------------------
 * 
 * Set the bright level of the screen
 * 
 * Arguments:
 * 	-level: 0 to 100 brigh level
 * 
 * Returns: Nothing
 * 
 */
void backlight_set(uint8_t level);

/*
 * Function:  backlight_get 
 * --------------------
 * 
 * Return the actual backlight level
 * 
 * Returns: The bright level of the display 0-100
 * 
 */
uint8_t backlight_get();