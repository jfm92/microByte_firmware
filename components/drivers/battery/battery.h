#include "stdint.h"


void battery_init(void);
void battery_game_mode(bool game_mode);
uint8_t battery_get_percentage();
void batteryTask(void *arg);