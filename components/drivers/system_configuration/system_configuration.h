
/********************************
 *      Screen Configuration
 * ******************************/

#define SCR_MODEL       ST7789
#define SCR_WIDTH       240
#define SCR_HEIGHT      240
#define SCR_BACKLIGTH   FALSE

#define HSPI_MOSI        13
#define HSPI_CLK         14
#define HSPI_RST         33
#define HSPI_DC          27
#define HSPI_BACKLIGTH   -1
#define HSPI_CLK_SPEED   40*1000*1000

/********************************
 *      Button Configuration
 * ******************************/

#define BTN_UP          34
#define BTN_DOWN        35
#define BTN_RIGHT       25
#define BTN_LEFT        32
#define BTN_A           26
#define BTN_B           12
#define BTN_START       2
#define BTN_SELECT      15

/********************************
 *   Flash memory Configuration
 * ******************************/
//TODO: Add all the GPIO
#define VSPI_MOSI       23 
#define VSPI_MISO       19 
#define VSPI_CLK        18        
#define VSPI_CS0        5                  
#define VSPI_CLK_SPEED  40*1000*1000
#define FLASH_SIZE      128     //128 MBytes

/********************************
 *      System Features
 * ******************************/

#define SOUND_EN        FALSE
#define INIT_MENU_EN    TRUE
#define SD_CARD_EN      FALSE