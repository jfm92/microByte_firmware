#include "stdbool.h"
#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/************ Queue *************/
QueueHandle_t modeQueue;
QueueHandle_t batteryQueue;

#define MODE_GAME           0x01
#define MODE_SAVE_GAME      0x02
#define MODE_LOAD_GAME      0x03
#define MODE_EXT_APP        0x04
#define MODE_UPDATE         0x05
#define MODE_OUT            0x06
#define MODE_BATTERY_ALERT  0x07
#define MODE_CHANGE_VOLUME  0x08
#define MODE_CHANGE_BRIGHT  0x09


#define GAMEBOY         0x00
#define GAMEBOY_COLOR   0x01
#define NES             0x02
#define SNES            0x03
#define SMS             0x04
#define GG              0x005

#define MEMORY_DMA 0x00
#define MEMORY_INTERNAL 0x01
#define MEMORY_SPIRAM 0X02
#define MEMORY_ALL 0x03

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

char app_version[32];
char idf_version[32];
char cpu_version[32];
uint32_t RAM_size;
uint32_t FLASH_size;

int system_memory(uint8_t memory);
void system_info();
