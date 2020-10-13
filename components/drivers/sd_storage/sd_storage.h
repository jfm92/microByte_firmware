#include "stdint.h"
#include "stdbool.h"

#define SDIO    0x00
#define MMC     0x01
#define SDHC    0x02
#define SDSC    0x03

struct sd_card_info{
    char card_name[32];
    uint8_t card_type;
    uint16_t card_size;
    uint32_t card_speed;
};

extern struct sd_card_info sd_card_info;

uint8_t sd_init();
void sd_format();
void sd_unmount();
void sd_mount();
uint8_t sd_game_list(char game_name[30][100], uint8_t console);
size_t sd_file_size(const char *path);
void  sd_get_file (const char *path, void * data);
bool sd_mounted();