#include "stdint.h"

#define IN 0
#define OUT 1

#define CONFIG_REG 0x06
#define READ_REG 0x00 

int8_t TCA955_init(void);
int8_t TCA9555_pinMode(uint8_t pin, uint8_t mode);
int16_t TCA9555_readInputs(void);