#define MSG_LOW_BATTERY_GAME    0x00
#define MSG_LOW_BATTERY         0x01


void GUI_task(void *arg);
void GUI_refresh();
void GUI_async_message();
