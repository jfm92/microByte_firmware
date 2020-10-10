#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"

#include "GUI.h"
#include "st7789.h"
#include "GUI_frontend.h"

#include "lvgl/lvgl.h"

/*********************
 *      DEFINES
 *********************/
#define LV_TICK_PERIOD_MS 10
#define DISP_BUF_SIZE   240*40 // Horizontal Res * 40 vetical pixels

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void lv_tick_task(void *arg);

/**********************
 *  STATIC VARIABLES
 **********************/
static SemaphoreHandle_t xGuiSemaphore;

/**********************
*   GLOBAL FUNCTIONS
**********************/

void GUI_init(void){

    xGuiSemaphore = xSemaphoreCreateMutex();

    // LVGL Initialization
    lv_init();

    //Screen Buffer initialization
    static EXT_RAM_ATTR lv_color_t * buf1[DISP_BUF_SIZE];
    //static lv_color_t buf2[DISP_BUF_SIZE];

    static lv_disp_buf_t disp_buf;
    uint32_t size_in_px = DISP_BUF_SIZE; 

    lv_disp_buf_init(&disp_buf, buf1, NULL, size_in_px);

    // Initialize LVGL display and attach the flush function
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = st7789_flush;

    disp_drv.buffer = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    // Create timer to handle LVGL system tick
    const esp_timer_create_args_t periodic_timer_args = {
            .callback = &lv_tick_task,
            .name = "periodic_gui"
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    //On ESP32 it's better to create a periodic task instead of esp_register_freertos_tick_hook
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, LV_TICK_PERIOD_MS * 1000)); // LV_TICK_PERIOD_MS expressed as microseconds

    // Init menu graphic user interface
    GUI_frontend();

}

void GUI_task(void *arg){

    GUI_init();

    while (1) {
        lv_task_handler();
    }
    printf("delete\r\n");
    //A task should NEVER return
    vTaskDelete(NULL);
}

/**********************
*   STATIC FUNCTIONS
**********************/

static void lv_tick_task(void *arg) {
    (void) arg;

    lv_tick_inc(LV_TICK_PERIOD_MS);
}

