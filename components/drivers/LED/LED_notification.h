#define LED_BLINK_HS    0x00
#define LED_BLINK_LS    0x01
#define LED_TURN_ON     0x02
#define LED_TURN_OFF    0x03

void LED_init();
void LED_mode(uint8_t mode);