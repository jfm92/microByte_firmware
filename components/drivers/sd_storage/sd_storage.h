void sd_init();
void sd_format();
void sd_unmount();
void sd_mount();
uint8_t sd_game_list(char game_name[30][100], uint8_t console);
void IRAM_ATTR sd_get_file (const char *path, void * data);