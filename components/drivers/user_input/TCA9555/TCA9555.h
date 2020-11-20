
/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  TCA955_init 
 * --------------------
 * 
 * Initialize I2C Bus and check if the TCA955 mux is connected to the bus
 * 
 * Returns: True if the initialization suceed otherwise false.
 * 
 */
bool TCA955_init(void);

/*
 * Function:  TCA9555_pinMode 
 * --------------------
 * 
 * Configure a specific pin to be an input.
 * By default all the pins are inputs.
 * 
 * Arguments
 * -pin: 0-16 pin number.
 * 
 * Returns: True if the configuration suceed otherwise false.
 * 
 */
bool TCA9555_pinMode(uint8_t pin);

/*
 * Function:  TCA9555_readInputs 
 * --------------------
 * 
 * Get the value of the input pins attached to the driver.
 * 
 * 
 * Returns: A integer of 16 bits with the value of each pin (Each bit is the value of each input).
 * 
 */
int16_t TCA9555_readInputs(void);