#ifndef EMUAPI_H
#define EMUAPI_H
#include <stdint.h>
#include <stdio.h>

#define HAS_SND     1
#define CUSTOM_SND  1
//#define TIMER_REND  1


//#if defined(ADAFRUIT_PYGAMER_ADVANCE_M4_EXPRESS)
// 320x240 display + SAMD51J20
  #define HIGH_SPEED_SPI                 // Only for ST7789's - the 7735 doesnt like it!
  #define EMU_SCALEDOWN       1
  #define USE_FLASH_FOR_ROMSTORAGE       // we need almost all the RAM for the framebuffer
  #define DEFAULT_FLASH_ADDRESS 0x40000  // make sure this is after this programs memory
  //#define USE_SAVEFILES
  #define USE_SRAM
/*#elif defined(ADAFRUIT_PYGAMER_M4_EXPRESS) ||  defined(ADAFRUIT_PYBADGE_M4_EXPRESS)
  #define EMU_SCALEDOWN       2
  #define USE_FLASH_FOR_ROMSTORAGE       // slows it down, but bigger roms!
  #define DEFAULT_FLASH_ADDRESS (0x40000-2048)  // make sure this is after this programs memory, with unrolled loops we're at 222,192! we need a little more than 256KB since roms have 10 extra bytes
  #define USE_SAVEFILES
  #define USE_SRAM
#else 
  #error "Need to give some platform details!"
#endif*/

#define emu_Init(ROM) {nes_Start(ROM); nes_Init(); }
#define emu_Step(x) { nes_Step(); }

#define PALETTE_SIZE         256
#define VID_FRAME_SKIP       0x0
#define TFT_VBUFFER_YCROP    0
#define SINGLELINE_RENDERING 1

extern void emu_init(void);
extern void emu_printf(const char *format, ...);
extern void * emu_Malloc(int size);
extern void emu_Free(void * pt);
extern uint8_t *emu_LoadROM(char *filename);
extern int emu_FileOpen(char * filename);
extern int emu_FileRead(uint8_t * buf, int size);
extern unsigned char emu_FileGetc(void);
extern int emu_FileSeek(int seek);
extern void emu_FileClose(void);
extern int emu_FileSize(char * filename);
extern int emu_LoadFile(char * filename, char * buf, int size);
extern int emu_LoadFileSeek(char * filename, char * buf, int size, int seek);
extern void emu_LoadState(char skipMenu);
extern void emu_SaveState(void);
void emu_Halt(const char * error_msg);

extern void emu_SetPaletteEntry(unsigned char r, unsigned char g, unsigned char b, int index);
#if 0 // NOT CURRENTLY USED
extern void emu_DrawScreen(unsigned char * VBuf, int width, int height, int stride);
#endif
extern void emu_DrawLine(unsigned char * VBuf, int width, int height, int line);
extern void emu_DrawVsync(void);
extern int emu_FrameSkip(void);
extern void * emu_LineBuffer(int line);

extern uint16_t emu_DebounceLocalKeys(void);
extern uint32_t emu_ReadKeys(void);
extern void emu_sndPlaySound(int chan, int volume, int freq);
extern void emu_sndPlayBuzz(int size, int val);
extern void emu_sndInit();
extern void emu_resetus(void);
extern int emu_us(void);

#endif
