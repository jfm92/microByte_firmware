#ifndef __LOADER_H__
#define __LOADER_H__


typedef struct loader_s
{
	char *rom;
	char *base;
	char *sram;
	char *state;
	int ramloaded;
} loader_t;


extern loader_t loader;

void gbc_loader_unload();
int gbc_rom_load(const char *game_name);
int gbc_sram_load();
int gbc_sram_save();
void gbc_state_load(int n);
void gbc_state_save(int n);



#endif
