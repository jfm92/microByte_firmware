#include "stdbool.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/************ Queue *************/
QueueHandle_t modeQueue;
QueueHandle_t batteryQueue;

//Modes available
#define MODE_GAME           0x01
#define MODE_SAVE_GAME      0x02
#define MODE_LOAD_GAME      0x03
#define MODE_EXT_APP        0x04
#define MODE_UPDATE         0x05
#define MODE_OUT            0x06
#define MODE_BATTERY_ALERT  0x07
#define MODE_CHANGE_VOLUME  0x08
#define MODE_CHANGE_BRIGHT  0x09

//Emulators available to select
#define GAMEBOY         0x00
#define GAMEBOY_COLOR   0x01
#define NES             0x02
#define SNES            0x03
#define SMS             0x04
#define GG              0x005

//Memory types
#define MEMORY_DMA 0x00
#define MEMORY_INTERNAL 0x01
#define MEMORY_SPIRAM 0X02
#define MEMORY_ALL 0x03

//System States
#define SYS_NORMAL_STATE 0x00
#define SYS_SOFT_RESET 0x01

//System configuration variables
#define SYS_BRIGHT 0x00
#define SYS_VOLUME 0x01
#define SYS_GUI_COLOR 0x02

// Struct to send data from emulator or inner stuff to the main control loop
struct SYSTEM_MODE{
    uint8_t mode;
    uint8_t status;
    uint8_t console;
    uint8_t load_save_game;
    uint8_t volume_level;
    uint8_t brightness_level;
    char game_name[200];
};

// Struct to save the battery level
struct BATTERY_STATUS{
    uint8_t percentage;
    uint32_t voltage;
};

//Variables to save machine data
char app_version[32];
char idf_version[32];
char cpu_version[32];
uint32_t RAM_size;
uint32_t FLASH_size;

/*
 * Function:  system_memory
 * --------------------
 * 
 * Return free memory available
 * 
* Arguments:
 * 	-memory: Select which tipe of memory you want to check
 * 
 * Returns: The free available memory.
 * 
 */
int system_memory(uint8_t memory);

/*
 * Function:  System Info
 * --------------------
 * 
 * Get the next system information:
 *  - Firmware version.
 *  - CPU version.
 *  - Total Ram size.
 *  - Total Flash size.
 * 
* Arguments:
 * 	-memory: Select which tipe of memory you want to check
 * 
 * Returns: Nothing
 * 
 */
void system_info();

/*
 * Function:  system_init_config
 * --------------------
 * 
 * Initialize non volatile storage to get or storage config
 * 
* Arguments:
 * 	Nothing
 * 
 * Returns: Nothing
 * 
 */
void system_init_config();

/*
 * Function:  system_set_state
 * --------------------
 * 
 * Save the last machine status.
 * 
* Arguments:
 * 	- State: State of the machine.
 *      - SYS_NORMAL_STATE -> Nothing special.
 *      - SYS_SOFT_RESET -> The machine has been reset by a soft reset.
 * 
 * Returns: Nothing
 * 
 */
void system_set_state(int8_t state);

/*
 * Function:  system_get_state
 * --------------------
 * 
 * Return the last saved machine status.
 * 
* Arguments:
* 	Nothing
 * 
 * Returns:
 *      - SYS_NORMAL_STATE(0): Basic state, nothing has append.
 *      - SYS_SOFT_RESET(1): The machine has been reset by a soft reset.
 * 
 */
int8_t system_get_state();

/*
 * Function:  system_save_config
 * --------------------
 * 
 * Save system configuration such as audio level, brightness and theme selected.
 * 
* Arguments:
 * 	- Config: Which configuration do you want to save.
 *      - SYS_VOLUME -> Modify audio volume.
 *      - SYS_BRIGHT -> Modify screen brightness.
 *      - SYS_GUI_COLOR -> Modify theme color of the GUI.
 *  - value: Value of the configuration that you want to save.
 *      - SYS_VOLUME -> 0  to 100
 *      - SYS_BRIGHT -> 1 to 100
 *      - SYS_GUI_COLOR -> 0(Light Theme) or 1(Dark Theme)
 * 
 * Returns: Nothing
 * 
 */
void system_save_config(uint8_t config, int8_t value);

/*
 * Function:  system_get_config
 * --------------------
 * 
 *  Get saved configuration on the NVS system
 * 
* Arguments:
 * 	- Config: Which configuration do you want to save.
 *      - SYS_VOLUME -> Get audio volume.
 *      - SYS_BRIGHT -> GET screen brightness.
 *      - SYS_GUI_COLOR -> Get theme color of the GUI.
 * 
 * Returns: Value of the configuration.
 * 
 */
int8_t system_get_config(uint8_t config);