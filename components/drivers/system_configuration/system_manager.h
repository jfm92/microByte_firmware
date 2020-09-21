#include "stdbool.h"

/************ Queue *************/
QueueHandle_t modeQueue;
QueueHandle_t batteryQueue;

#define MODE_GAME 0x01
#define MODE_SAVE_GAME 0x02
#define MODE_LOAD_GAME 0x03
#define MODE_WIFI_LIB 0x04
#define MODE_BT_CONTROLLER 0x05
#define MODE_OUT 0x06
#define MODE_BATTERY_ALERT 0x07


#define GAMEBOY 0x00
#define GAMEBOY_COLOR 0x01
#define NES 0x02
#define SNES 0x03
#define SMS 0x03

struct SYSTEM_MODE{
    uint8_t mode;
    uint8_t status;
    uint8_t console;
    uint8_t volume_level;
    uint8_t brightness_level;
    char game_name[200];
};

struct BATTERY_STATUS{
    uint8_t percentage;
    uint32_t voltage;
};

