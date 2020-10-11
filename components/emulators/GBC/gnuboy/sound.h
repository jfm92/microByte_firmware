#ifndef __SOUND_H__
#define __SOUND_H__


struct sndchan
{
	int on;
	unsigned pos;
	int cnt, encnt, swcnt;
	int len, enlen, swlen;
	int swfreq;
	int freq;
	int envol, endir;
};


struct snd
{
	int rate;
	struct sndchan ch[4];
	uint8_t wave[16];
};


extern struct snd snd;

void sound_write(uint8_t r, uint8_t b);
uint8_t sound_read(uint8_t r);
void sound_dirty();
void gbc_sound_reset();
void sound_mix();

#endif
