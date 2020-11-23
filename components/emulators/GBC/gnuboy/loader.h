#ifndef __LOADER_H__
#define __LOADER_H__
#include <stdbool.h>

typedef struct loader_s
{
	char *rom;
	char *base;
	char *sram;
	char *state;
	int ramloaded;
} loader_t;


extern loader_t loader;

bool gbc_rom_load(const char *game_name, uint8_t console);
int gbc_sram_load();
int gbc_sram_save();
bool gbc_state_load(const char *game_name, uint8_t console);
bool gbc_state_save(const char *game_name, uint8_t console);



#endif
