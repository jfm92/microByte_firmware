/********************************
 *    Display pin configuration
 * ******************************/

#define SCR_MODEL       ST7789
#define SCR_WIDTH       240
#define SCR_HEIGHT      240

#define HSPI_MOSI        13
#define HSPI_CLK         14
#define HSPI_RST         33
#define HSPI_DC          32
#define HSPI_BACKLIGTH   15
#define HSPI_CLK_SPEED   40*1000*1000


/********************************
 *   SD CARD pin configuration
 * ******************************/

#define VSPI_MOSI       23 
#define VSPI_MISO       19 
#define VSPI_CLK        18        
#define VSPI_CS0        5                  
#define VSPI_CLK_SPEED  18*1000*1000

/********************************
 *   Pin Mux pin configuration
 * ******************************/

#define MUX_SDA         21
#define MUX_SCL         22
#define MUX_INT         34
#define I2C_CLK         400*1000
#define I2C_dev_address 0x20