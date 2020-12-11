//LED commands available
#define LED_BLINK_HS    0x00 // Blink High Speed -> Period of 100 mS
#define LED_BLINK_LS    0x01 // Blink Low Speed ->  Period of 500 mS  
#define LED_TURN_ON     0x02
#define LED_TURN_OFF    0x03
#define LED_FADE_ON     0x04
#define LED_FADE_OFF    0x05
#define LED_LOAD_ANI    0x06

/*
 * Function:  LED_init 
 * --------------------
 * 
 * Initialize the notification LED control
 * 
 * 
 * Returns: Nothing
 * 
 */
void LED_init();

/*
 * Function:  LED_mode 
 * --------------------
 * 
 * Set the blink/fade mode of the LED.
 * 
* Arguments:
 * 	-mode: Set the mode that you want to run, see defines up
 * 
 * Returns: Nothing
 * 
 */
void LED_mode(uint8_t mode);