#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define INPUT_CHANGE 0x01

QueueHandle_t input_queue;

void input_init(void);
uint16_t input_read(void);