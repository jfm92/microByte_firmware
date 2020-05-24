#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rtc_io.h"
//#include "nvs_flash.h"

#include <loader.h>
#include <hw.h>
#include <lcd.h>
#include <fb.h>
#include <cpu.h>
//#include <mem.h>
#include <pcm.h>
#include <regs.h>
#include <rtc.h>
#include <gnuboy.h>

#include <string.h>

#include "display_hal.h"
#include "gamepad.h"

//Video qeue
QueueHandle_t vidQueue;
QueueHandle_t fpsQueue;


struct fb fb;
struct pcm pcm;

uint16_t *displayBuffer[2];
uint8_t currentBuffer;

uint16_t *framebuffer;
int frame = 0;
uint elapsedTime = 0;

volatile bool videoTaskIsRunning = false;

void videoTask(void *arg)
{
    printf("Init videoTask\r\n");
    display_init();

    display_gb_frame(NULL);


    esp_err_t ret;
    videoTaskIsRunning = true;

    uint16_t *param;
    while (1)
    {
        xQueuePeek(vidQueue, &param, portMAX_DELAY);

        if (param == 1)
            break;

        display_gb_frame(param);

        xQueueReceive(vidQueue, &param, portMAX_DELAY);
        vTaskDelay(2 / portTICK_RATE_MS);
    }

    videoTaskIsRunning = false;
    vTaskDelete(NULL);

    while (1)
    {
    }
}

void run_to_vblank()
{
    /* FRAME BEGIN */

    /* FIXME: djudging by the time specified this was intended
  to emulate through vblank phase which is handled at the
  end of the loop. */
    cpu_emulate(2280);

    /* FIXME: R_LY >= 0; comparsion to zero can also be removed
  altogether, R_LY is always 0 at this point */
    while (R_LY > 0 && R_LY < 144)
    {
        /* Step through visible line scanning phase */
        emu_step();
    }

    /* VBLANK BEGIN */

    //vid_end();
    if ((frame % 2) == 0)
    {
        xQueueSend(vidQueue, &framebuffer, portMAX_DELAY);

        // swap buffers
        currentBuffer = currentBuffer ? 0 : 1;
        framebuffer = displayBuffer[currentBuffer];

        fb.ptr = framebuffer;
    }

    rtc_tick();

    //sound_mix();

   /* if (pcm.pos > 100)
    {
        currentAudioBufferPtr = audioBuffer[currentAudioBuffer];
        currentAudioSampleCount = pcm.pos;

        void *tempPtr = 0x1234;
        xQueueSend(audioQueue, &tempPtr, portMAX_DELAY);

        // Swap buffers
        currentAudioBuffer = currentAudioBuffer ? 0 : 1;
        pcm.buf = audioBuffer[currentAudioBuffer];
        pcm.pos = 0;
    }*/

    if (!(R_LCDC & 0x80))
    {
        /* LCDC operation stopped */
        /* FIXME: djudging by the time specified, this is
    intended to emulate through visible line scanning
    phase, even though we are already at vblank here */
        cpu_emulate(32832);
    }

    while (R_LY > 0)
    {
        /* Step through vblank phase */
        emu_step();
    }
}

void gnu_core_task(void *arg){
    displayBuffer[0] = heap_caps_malloc(160 * 144 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    displayBuffer[1] = heap_caps_malloc(160 * 144 * 2, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (displayBuffer[0] == 0 || displayBuffer[1] == 0)
        abort();

    framebuffer = displayBuffer[0];

    for (int i = 0; i < 2; ++i)
    {
        memset(displayBuffer[i], 0, 160 * 144 * 2);
    }

    emu_reset();

    rtc.d = 1;
    rtc.h = 1;
    rtc.m = 1;
    rtc.s = 1;
    rtc.t = 1;

    // vid_begin
    memset(&fb, 0, sizeof(fb));
    fb.w = 160;
    fb.h = 144;
    fb.pelsize = 2;
    fb.pitch = fb.w * fb.pelsize;
    fb.indexed = 0;
    fb.ptr = framebuffer;
    fb.enabled = 1;
    fb.dirty = 0;

    lcd_begin();

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    uint actualFrameCount = 0;

    //gamepad_init();

    while(1){
        //pad_set(PAD_START, gamepad_state());

        startTime = xthal_get_ccount();
        run_to_vblank();
        stopTime = xthal_get_ccount();
      

        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
        ++frame;
        ++actualFrameCount;

        if (actualFrameCount == 60)
        {
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f); // 240000000.0f; // (240Mhz)
            float fps = actualFrameCount / seconds;

            printf("HEAP:0x%x, FPS:%f\n", esp_get_free_heap_size(), fps);

            actualFrameCount = 0;
            totalElapsedTime = 0;
        }
        
    }
}

void app_main(void){
    printf("Init gnuBoy\r\n");

    loader_init(NULL);



    printf("app_main: displayBuffer[0]=%p, [1]=%p\n", displayBuffer[0], displayBuffer[1]);

    vidQueue = xQueueCreate(1, sizeof(uint16_t *));

    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(&gamepad_task, "gamepadTask", 2048,NULL,2,NULL,1);
    vTaskDelay(10 / portTICK_RATE_MS);
    xTaskCreatePinnedToCore(&gnu_core_task, "gnu_core_task", 3048, NULL, 1, NULL, 0);

}