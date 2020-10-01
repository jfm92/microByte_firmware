#include "stdbool.h"
#include "emuapi.h"
#include  "nes_emu.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sd_storage.h"
#include "display_hal.h"
#include "scr_spi.h"


unsigned short palette16[PALETTE_SIZE];
#define RGBVAL16(r,g,b)  ( (((r>>3)&0x1f)<<11) | (((g>>2)&0x3f)<<5) | (((b>>3)&0x1f)<<0) )
static void NESTask(void *arg);
int skip=0;
volatile bool vbl=true;

int emu_FrameSkip(void)
{
  return 1;
}

void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index)
{
  if (index<PALETTE_SIZE) {
    //Serial.println("%d: %d %d %d\n", index, r,g,b);
#if EMU_SCALEDOWN == 1
    palette16[index] = __builtin_bswap16(RGBVAL16(r,g,b));
#elif EMU_SCALEDOWN == 2
    // 00RRRRRRRR000BBBBBBBB00GGGGGGGG0
    palette32[index] =
      ((uint32_t)r << 22) | ((uint32_t)b << 11) | ((uint32_t)g << 1);
#endif
  }
}

void emu_DrawLine(unsigned char * VBuf, int width, int height, int line) 
{
 /*  
  if (line == 0) {
    //digitalWrite(FRAME_LED, frametoggle);
    //frametoggle = !frametoggle;
  }
#if EMU_SCALEDOWN == 1
 // tft.writeLine(width, 1, line, VBuf, palette16);
 printf("print frame\r\n");
 //display_gb_frame(VBuf);
 scr_spi_send_lines(width, 1, width, VBuf, line);
#elif EMU_SCALEDOWN == 2
  //tft.writeLine(width, 1, line, VBuf, palette32);
  display_gb_frame(VBuf);
#endif*/

}



void emu_DrawVsync(void)
{
  volatile bool vb=vbl;
  skip += 1;
  skip &= VID_FRAME_SKIP;

  while (vbl==vb) {};
}
static void NESTask(void *arg){
    printf("Nestask created\r\n");
    while(1){
        printf("emustep\r\n");
        //emu_Step();
      
        //vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void NES_init(){
    // Task emulador

    emu_Init("hl");
    
  
   // sd_get_file("/sdcard/NES/mario.nes",data);
   //vTaskDelay(250 / portTICK_RATE_MS);
    // xTaskCreatePinnedToCore(&NESTask, "NESTask", 1024*2, NULL, 1, NULL, 1);
}

