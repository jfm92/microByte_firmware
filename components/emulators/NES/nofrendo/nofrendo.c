/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nofrendo.c
**
** Entry point of program
** Note: all architectures should call these functions
** $Id: nofrendo.c,v 1.3 2001/04/27 14:37:11 neil Exp $
*/

/*#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <nofrendo.h>
#include <event.h>
#include <nofconfig.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <noftypes.h>
#include "nes6502.h"
#include <log.h>
#include <osd.h>
#include <nes.h>
#include <nes_apu.h>
#include <nes_ppu.h>
#include <nes_rom.h>
#include <nes_mmc.h>
#include <vid_drv.h>


#include <freertos/FreeRTOS.h>
#include "esp_system.h"*/

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
#include <vid_drv.h>
#include <nofrendo.h>
#include <event.h>
#include <nofconfig.h>
#include <freertos/FreeRTOS.h>
#include "esp_system.h"

#define  NES_CLOCK_DIVIDER    12
//#define  NES_MASTER_CLOCK     21477272.727272727272
#define  NES_MASTER_CLOCK     (236250000 / 11)
#define  NES_SCANLINE_CYCLES  (1364.0 / NES_CLOCK_DIVIDER)
#define  NES_FIQ_PERIOD       (NES_MASTER_CLOCK / NES_CLOCK_DIVIDER / 60)

#define  NES_RAMSIZE          0x8000

#define  NES_SKIP_LIMIT       (NES_REFRESH_RATE / 5)   /* 12 or 10, depending on PAL/NTSC */

//static nes_t nes;
nes_t *nes;
/* our happy little timer ISR */
volatile int nofrendo_ticks = 0;
static void timer_isr(void)
{
   nofrendo_ticks++;
}


int nofrendo_main(int argc, char *argv[])
{
   if (log_init())
      return -1;

   event_init();

    vidinfo_t video;

   if (config.open())
      return -1;

   if (osd_init())
      return -1;

   //if (gui_init())
   //   return -1;

   osd_getvideoinfo(&video);
   if (vid_init(video.default_width, video.default_height, video.driver))
      return -1;

 
   /* set up the event system for this system type */
   event_set_system(system_nes);

   //gui_setrefresh(NES_REFRESH_RATE);

   nes = nes_create();
   if (NULL == nes)
   {
      log_printf("Failed to create NES instance.\n");
      return -1;
   }

   if (nes_insertcart("foo",nes))
      return -1;

   vid_setmode(NES_SCREEN_WIDTH, NES_VISIBLE_HEIGHT);

   osd_installtimer(NES_REFRESH_RATE, (void *) timer_isr);

   //nes_emulate();

   int last_ticks, frames_to_render;

    osd_setsound(nes->apu->process);

    last_ticks = nofrendo_ticks;
    frames_to_render = 0;
    nes->scanline_cycles = 0;
    nes->fiq_cycles = (int) NES_FIQ_PERIOD;

    uint startTime;
    uint stopTime;
    uint totalElapsedTime = 0;
    int frame = 0;
    int skipFrame = 0;


    for (int i = 0; i < 4; ++i)
    {
        nes_renderframe(1);
        system_video(1);
    }

    while (1)
    {
        startTime = xthal_get_ccount();

        bool renderFrame = ((skipFrame % 2) == 0);

        nes_renderframe(renderFrame);
        system_video(renderFrame);

        if (skipFrame % 7 == 0) ++skipFrame;
        ++skipFrame;

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

            printf("HEAP:0x%x, FPS:%f\n", esp_get_free_heap_size(), fps);

            frame = 0;
            totalElapsedTime = 0;
        }
    }

   return 0;
}


/*
** $Log: nofrendo.c,v $
** Revision 1.3  2001/04/27 14:37:11  neil
** wheeee
**
** Revision 1.2  2001/04/27 11:10:08  neil
** compile
**
** Revision 1.1.1.1  2001/04/27 07:03:54  neil
** initial
**
** Revision 1.48  2000/11/27 12:47:08  matt
** free them strings
**
** Revision 1.47  2000/11/25 20:26:05  matt
** removed fds "system"
**
** Revision 1.46  2000/11/25 01:51:53  matt
** bool stinks sometimes
**
** Revision 1.45  2000/11/20 13:22:12  matt
** standardized timer ISR, added nofrendo_ticks
**
** Revision 1.44  2000/11/05 22:53:13  matt
** only one video driver per system, please
**
** Revision 1.43  2000/11/01 14:15:35  matt
** multi-system event system, or whatever
**
** Revision 1.42  2000/10/28 15:16:24  matt
** removed nsf_init
**
** Revision 1.41  2000/10/27 12:58:44  matt
** gui_init can now fail
**
** Revision 1.40  2000/10/26 22:48:57  matt
** prelim NSF support
**
** Revision 1.39  2000/10/25 13:42:02  matt
** strdup - giddyap!
**
** Revision 1.38  2000/10/25 01:23:08  matt
** basic system autodetection
**
** Revision 1.37  2000/10/23 17:50:47  matt
** adding fds support
**
** Revision 1.36  2000/10/23 15:52:04  matt
** better system handling
**
** Revision 1.35  2000/10/21 19:25:59  matt
** many more cleanups
**
** Revision 1.34  2000/10/10 13:58:13  matt
** stroustrup squeezing his way in the door
**
** Revision 1.33  2000/10/10 13:03:54  matt
** Mr. Clean makes a guest appearance
**
** Revision 1.32  2000/09/15 04:58:06  matt
** simplifying and optimizing APU core
**
** Revision 1.31  2000/09/10 23:19:14  matt
** i'm a sloppy coder
**
** Revision 1.30  2000/09/07 01:30:57  matt
** nes6502_init deprecated
**
** Revision 1.29  2000/08/16 03:17:49  matt
** bpb
**
** Revision 1.28  2000/08/16 02:58:19  matt
** changed video interface a wee bit
**
** Revision 1.27  2000/07/31 04:28:46  matt
** one million cleanups
**
** Revision 1.26  2000/07/30 04:31:26  matt
** automagic loading of the nofrendo intro
**
** Revision 1.25  2000/07/27 01:16:36  matt
** sorted out the video problems
**
** Revision 1.24  2000/07/26 21:54:53  neil
** eject has to clear the nextfilename and nextsystem
**
** Revision 1.23  2000/07/26 21:36:13  neil
** Big honkin' change -- see the mailing list
**
** Revision 1.22  2000/07/25 02:21:36  matt
** safer xxx_destroy calls
**
** Revision 1.21  2000/07/23 16:46:47  matt
** fixed crash in win32 by reodering shutdown calls
**
** Revision 1.20  2000/07/23 15:18:23  matt
** removed unistd.h from includes
**
** Revision 1.19  2000/07/23 00:48:15  neil
** Win32 SDL
**
** Revision 1.18  2000/07/21 13:42:06  neil
** get_options removed, as it should be handled by osd_main
**
** Revision 1.17  2000/07/21 04:53:48  matt
** moved palette calls out of nofrendo.c and into ppu_create
**
** Revision 1.16  2000/07/21 02:40:43  neil
** more main fixes
**
** Revision 1.15  2000/07/21 02:09:07  neil
** new main structure?
**
** Revision 1.14  2000/07/20 17:05:12  neil
** Moved osd_init before setup_video
**
** Revision 1.13  2000/07/11 15:01:05  matt
** moved config.close() into registered atexit() routine
**
** Revision 1.12  2000/07/11 13:35:38  bsittler
** Changed the config API, implemented config file "nofrendo.cfg". The
** GGI drivers use the group [GGI]. Visual= and Mode= keys are understood.
**
** Revision 1.11  2000/07/11 04:32:21  matt
** less magic number nastiness for screen dimensions
**
** Revision 1.10  2000/07/10 03:04:15  matt
** removed scanlines, backbuffer from custom blit
**
** Revision 1.9  2000/07/07 04:39:54  matt
** removed garbage dpp shite
**
** Revision 1.8  2000/07/06 16:48:25  matt
** new video driver
**
** Revision 1.7  2000/07/05 17:26:16  neil
** Moved the externs in nofrendo.c to osd.h
**
** Revision 1.6  2000/06/26 04:55:44  matt
** cleaned up main()
**
** Revision 1.5  2000/06/20 20:41:21  matt
** moved <stdlib.h> include to top (duh)
**
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/
