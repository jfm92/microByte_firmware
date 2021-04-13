
/*********************
 *      LIBRARIES
 *********************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <noftypes.h>
#include "nes6502.h"
#include <log.h>
#include <osd.h>
#include <gui.h>
#include <nes.h>
#include <nes_apu.h>
#include <nes_ppu.h>
#include <nes_rom.h>
#include <nes_mmc.h>
#include <nesinput.h>
#include <vid_drv.h>
#include <nofrendo.h>
#include <event.h>
#include <nofconfig.h>

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "esp_system.h"
#include "esp_log.h"

#include "sd_storage.h"
#include "display_HAL.h"
#include "user_input.h"
#include "sound_driver.h"
#include "system_manager.h"


/*********************
 *      DEFINES
 *********************/
char game_route[256];
#define DEFAULT_SAMPLERATE 16000
#define DEFAULT_FRAGSIZE 128

#define DEFAULT_WIDTH 240
#define DEFAULT_HEIGHT 240

#define  NES_CLOCK_DIVIDER    12
//#define  NES_MASTER_CLOCK     21477272.727272727272
#define  NES_MASTER_CLOCK     (236250000 / 11)
#define  NES_SCANLINE_CYCLES  (1364.0 / NES_CLOCK_DIVIDER)
#define  NES_FIQ_PERIOD       (NES_MASTER_CLOCK / NES_CLOCK_DIVIDER / 60)

#define  NES_RAMSIZE          0x800

#define  NES_SKIP_LIMIT       (NES_REFRESH_RATE / 5)   /* 12 or 10, depending on PAL/NTSC */

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void (*audio_callback)(void *buffer, int length) = NULL;

static void audioTask(void *arg);
static void videoTask(void *arg);
static void nofrendoTask(void *arg);

static void load_game(const char *game_name);

static int init(int width, int height);
static void shutdown(void);
static int set_mode(int width, int height);
static void clear(uint8 color);
static void set_palette(rgb_t *pal);
static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects);
static bitmap_t *lock_write(void);
static void free_write(int num_dirties, rect_t *dirty_rects);

static void do_audio_frame();

static void timer_isr(void);

static nes_t *nes;


/**********************
 *  TASK & TIMER HANDLERS
 **********************/
TimerHandle_t timer;
TaskHandle_t videoTask_handler;
TaskHandle_t audioTask_handler;
TaskHandle_t nofrendoTask_handler;

/**********************
 *      STRUCTS
 **********************/
viddriver_t sdlDriver =
	{
		"Simple DirectMedia Layer", // name 
		init,						// init 
		shutdown,					// shutdown 
		set_mode,					// set_mode 
		set_palette,				// set_palette 
		clear,						// clear 
		lock_write,					// lock_write 
		free_write,					// free_write 
		custom_blit,				// custom_blit 
		false						// invalidate flag 
};

/**********************
 *  QUEUE HANDLERS
 **********************/

QueueHandle_t vidQueue;
QueueHandle_t audioQueue;

/**********************
 *   GLOBAL VARIABLES
 **********************/

static int16_t *audio_frame;
int audioFrame;

uint16 myPalette[256];

static char fb[1]; //dummy
bitmap_t *myBitmap;
char* data = NULL;
volatile int nofrendo_ticks = 0;

static const char *TAG = "NES_manager";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void NES_start(){

   ESP_LOGI(TAG,"Executing NES(nonfrendo) emulator");

    // Queue creation
    vidQueue = xQueueCreate(10, sizeof(bitmap_t *));
    audioQueue = xQueueCreate(1, sizeof(uint16_t *));

    //Execute emulator tasks.
    xTaskCreatePinnedToCore(&videoTask, "videoTask", 2048, NULL, 4, &videoTask_handler, 0);
    //xTaskCreatePinnedToCore(&audioTask, "audioTask", 2048, NULL, 1, &audioTask_handler, 1);
    xTaskCreatePinnedToCore(&nofrendoTask, "nofrendoTask", 1024*5, NULL, 1, &nofrendoTask_handler, 1);

}

void NES_resume(){
    ESP_LOGI(TAG,"NES Resume");
    vTaskResume(videoTask_handler);
    //vTaskResume(audioTask_handler);
    vTaskResume(nofrendoTask_handler);
}

void NES_suspend(){
    ESP_LOGI(TAG,"NES Suspend");
    vTaskSuspend(nofrendoTask_handler);
    vTaskSuspend(videoTask_handler);
    //vTaskSuspend(audioTask_handler);
}

void NES_load_game(const char *game_name){
   ESP_LOGI(TAG,"NES loading ROM: %s",game_name);

    
	sprintf(game_route,"/sdcard/NES/%s",game_name);
    config.filename = game_route;

    //Get the file size
    /*size_t game_size = sd_file_size(game_route);

    //Allocate the memory and clean it.
	data = malloc(game_size);
	memset(data,0,game_size);

	sd_get_file(game_route,data);*/

    //TODO: Add load save state.

}

void NES_save_game(){
    //TODO: Implement save state
}
/**********************
 *   STATIC FUNCTIONS
 **********************/

static void timer_isr(void){
   nofrendo_ticks++;
}

static void nofrendoTask(void *arg){
    if (log_init()) return -1;

   event_init();

    vidinfo_t video;

   if (config.open()) return -1;

   if (osd_init()) return -1;

   osd_getvideoinfo(&video);
   if (vid_init(video.default_width, video.default_height, video.driver)) return -1;

 
   /* set up the event system for this system type */
   event_set_system(system_nes);

   nes = nes_create();
   if (NULL == nes){
	   ESP_LOGE(TAG,"nes_create fail");
      return -1;
   }

   if (nes_insertcart(game_route,nes)) return -1;

   vid_setmode(NES_SCREEN_WIDTH, NES_VISIBLE_HEIGHT);

   osd_installtimer(NES_REFRESH_RATE, (void *) timer_isr);

   int last_ticks, frames_to_render;

    osd_setsound(nes->apu->process);

    last_ticks = nofrendo_ticks;
    frames_to_render = 0;
    nes->scanline_cycles = 1;
    nes->fiq_cycles = (int) NES_FIQ_PERIOD;

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    int skipFrame = 0;


    for (int i = 0; i < 4; ++i){
        nes_renderframe(1);
        system_video(1);
    }

    while (1){
        startTime = xthal_get_ccount();

        bool renderFrame;
        if(skipFrame % 2 == 0){
            skipFrame++;
            renderFrame = false;
        }
        else{
            skipFrame = 0;
            renderFrame = true;
        }

        nes_renderframe(renderFrame);
        system_video(renderFrame);

        //do_audio_frame();
        
        stopTime = xthal_get_ccount();

        int elapsedTime;
        if (stopTime > startTime)
            elapsedTime = (stopTime - startTime);
        else
            elapsedTime = ((uint64_t)stopTime + (uint64_t)0xffffffff) - (startTime);

        totalElapsedTime += elapsedTime;
         ++frame;
        

        if (frame == 60)
        {
            float seconds = totalElapsedTime / (CONFIG_ESP32_DEFAULT_CPU_FREQ_MHZ * 1000000.0f);
            float fps = frame / seconds;

            printf("FPS:%f\n",fps);

            frame = 0;
            totalElapsedTime = 0;
        }
    }
}

char *osd_getromdata() {
    printf("Initialized. ROM@%p\n", data);
    return (char*)data;
}


// Video functions 
static void videoTask(void *arg){
    ESP_LOGI(TAG, "nofrendo Video Task Initialize");
	bitmap_t *bmp = NULL;
    display_HAL_NES_frame(NULL);
	while (1){
		xQueueReceive(vidQueue, &bmp, portMAX_DELAY);
		display_HAL_NES_frame((const uint8_t **)bmp->line[0]);
	}
}

static int init(int width, int height){
    //Useless only here to avoid compilation errors
	return 0;
}

static void shutdown(void){}//Useless only here to avoid compilation errors

static int set_mode(int width, int height){
    //Useless only here to avoid compilation errors
	return 0;
}

static void clear(uint8 color){} //Useless only here to avoid compilation errors

static void set_palette(rgb_t *pal){
	uint16 c;
	for (int i = 0; i < 256; i++){
		c=(pal[i].b>>3)+((pal[i].g>>2)<<5)+((pal[i].r>>3)<<11);
        myPalette[i]=(c>>8)|((c&0xff)<<8);
	}
}

void osd_getvideoinfo(vidinfo_t *info){
	info->default_width = DEFAULT_WIDTH;
	info->default_height = DEFAULT_HEIGHT;
	info->driver = &sdlDriver;
}

static bitmap_t *lock_write(void){
	//   SDL_LockSurface(mySurface);
	myBitmap = bmp_createhw((uint8 *)fb, DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_WIDTH * 2);
	return myBitmap;
}

static void free_write(int num_dirties, rect_t *dirty_rects){
	bmp_destroy(&myBitmap);
}

static void custom_blit(bitmap_t *bmp, int num_dirties, rect_t *dirty_rects){
	xQueueSend(vidQueue, &bmp, 0);	
}


// Audio functions
static void audioTask(void *arg){
    
    ESP_LOGI(TAG, "nofrendo Audio Task Initialize");
    uint16_t *param;

    while(1){
		xQueueReceive(audioQueue, &param, portMAX_DELAY);
        do_audio_frame();
    }
}

static void do_audio_frame(){
	int left=DEFAULT_SAMPLERATE/NES_REFRESH_RATE;
    while(left) {
        int n=DEFAULT_FRAGSIZE;
        if (n>left) n=left;
        audio_callback(audio_frame, n);
        //16 bit mono -> 32-bit (16 bit r+l)
        for (int i=n-1; i>=0; i--)
        {
            int sample = (int)audio_frame[i];

            audio_frame[i*2]= (short)sample;
            audio_frame[i*2+1] = (short)sample;
        }
        audio_submit(audio_frame, n);
        left-=n;
    }
}

void osd_setsound(void (*playfunc)(void *buffer, int length)){
	//Indicates we should call playfunc() to get more data.
	audio_callback = playfunc;
}

static void osd_stopsound(void){
	audio_callback = NULL;
}

static int osd_init_sound(void){
	audio_frame = heap_caps_malloc(4 * DEFAULT_FRAGSIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
	audio_callback = NULL;
	return 0;
}

void osd_getsoundinfo(sndinfo_t *info){
	info->sample_rate = DEFAULT_SAMPLERATE;
	info->bps = 16;
}

int osd_installtimer(int frequency, void *func){
	ESP_LOGI(TAG,"Timer install, freq=%d\n", frequency);
	timer = xTimerCreate("nes", configTICK_RATE_HZ / frequency, pdTRUE, NULL, func);
	xTimerStart(timer, 0);
	return 0;
}

void osd_getinput(void)
{
	uint16_t b = input_read();

	const int ev[16] = {
		event_joypad1_down, event_joypad1_left, event_joypad1_up, event_joypad1_right, 0, 0, 0, 0,
		event_joypad1_b, event_joypad1_a, event_joypad1_start, 0, event_joypad1_select, 0, 0, 0};
	static int oldb = 0xffff;
	int chg = b ^ oldb;
	int x;
	oldb = b;
	event_t evh;

	for (x = 0; x < 16; x++)
	{
		if (chg & 1)
		{
			evh = event_get(ev[x]);
			if (evh)
				evh((b & 1) ? INP_STATE_BREAK : INP_STATE_MAKE);
		}
		chg >>= 1;
		b >>= 1;
	}
}

void osd_shutdown(){
	osd_stopsound();
}

static int logprint(const char *string){
	return printf("%s", string);
}

int osd_init(){
	printf("osd_init\r\n");
	osd_init_sound();
    return 0;
}

