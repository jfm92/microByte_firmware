/*
  Copyright Frank Bösing, 2017
  GPL license at https://github.com/FrankBoesing/Teensy64
*/

#ifndef _DISPLAY_DMA_
#define _DISPLAY_DMA_


//#include <Adafruit_Arcada.h>
//#include <Adafruit_ZeroDMA.h>

#ifdef __cplusplus
//#include <Arduino.h>
//#include <SPI.h>
#endif

#define DMA_FULL 1

#define RGBVAL16(r,g,b)  ( (((r>>3)&0x1f)<<11) | (((g>>2)&0x3f)<<5) | (((b>>3)&0x1f)<<0) )

#define NATIVE_WIDTH      256
#define NATIVE_HEIGHT     240
#define EMUDISPLAY_WIDTH   (NATIVE_WIDTH / EMU_SCALEDOWN)
#define EMUDISPLAY_HEIGHT  (NATIVE_HEIGHT / EMU_SCALEDOWN)

#ifdef __cplusplus

#define SCREEN_DMA_MAX_SIZE 0xD000
#define SCREEN_DMA_NUM_SETTINGS (((uint32_t)((2 * ARCADA_TFT_HEIGHT * ARCADA_TFT_WIDTH) / SCREEN_DMA_MAX_SIZE))+1)


class Display_DMA
{
  public:
  	Display_DMA(void) {};

	  void setFrameBuffer(uint16_t * fb);
	  static uint16_t * getFrameBuffer(void);

	  void refresh(void);
	  void stop();
	  void wait(void);	
    uint16_t * getLineBuffer(int j);
            
    void setArea(uint16_t x1,uint16_t y1,uint16_t x2,uint16_t y2); 
    void setAreaCentered(void);

#if 0 // NOT CURRENTLY BEING USED IN NOFRENDO. See notes in display_dma.cpp.
    void writeScreen(int width, int height, int stride, uint8_t *buffer, uint16_t *palette16);
#endif
#if EMU_SCALEDOWN == 1
    void writeLine(int width, int height, int stride, uint8_t *buffer, uint16_t *palette16);
#elif EMU_SCALEDOWN == 2
    void writeLine(int width, int height, int stride, uint8_t *buffer, uint32_t *palette32);
#endif
	  void fillScreen(uint16_t color);
	  void writeScreen(const uint16_t *pcolors);
	  void drawPixel(int16_t x, int16_t y, uint16_t color);
	  uint16_t getPixel(int16_t x, int16_t y);

    void dmaFrame(void); // End DMA-issued frame and start a new one
};

#endif
#endif
