/********************************
 *    Display pin configuration
 * ******************************/

#define SCR_MODEL       ST7789
#define SCR_WIDTH       240
#define SCR_HEIGHT      240
#define SCR_BUFFER_SIZE 20

#define HSPI_MOSI        13
#define HSPI_CLK         14
#define HSPI_RST         33
#define HSPI_DC          32
#define HSPI_BACKLIGTH   15
#define HSPI_CLK_SPEED   60*1000*1000 // 60Mhz


/********************************
 *   SD CARD pin configuration
 * ******************************/

#define VSPI_MOSI       23 
#define VSPI_MISO       19 
#define VSPI_CLK        18        
#define VSPI_CS0        5                  

/********************************
 *   Pin Mux pin configuration
 * ******************************/

#define MUX_SDA         21
#define MUX_SCL         22
#define MUX_INT         34
#define I2C_CLK         400*1000 //400Khz
#define I2C_dev_address 0x20

/********************************
 *          I2S Pin
 * ******************************/

#define I2S_BCK     27
#define I2S_WS      26
#define I2S_DATA_O  25
#define I2S_NUM I2S_NUM_0

/********************************
 *          Notification LED
 * ******************************/

#define LED_PIN     2