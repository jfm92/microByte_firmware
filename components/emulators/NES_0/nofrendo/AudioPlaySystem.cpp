#include "AudioPlaySystem.h"
//#include <Arduino.h>

extern "C" {
#include "emuapi.h"
}

//#define SAMPLERATE AUDIO_SAMPLE_RATE_EXACT
//#define CLOCKFREQ 985248
/*
volatile bool playing = false;

#ifdef CUSTOM_SND 
extern "C" {
  void SND_Process(void *sndbuffer, int sndn);
}


static void snd_Mixer(short *  stream, int len )
{
  if (playing)  {
    SND_Process((void*)stream, len);     
  }
}
#endif
  
void AudioPlaySystem::begin(void)
{
  //Serial.println("AudioPlaySystem constructor");
  
	this->reset();
	//setSampleParameters(CLOCKFREQ, SAMPLERATE);
}

void AudioPlaySystem::start(void)
{
  playing = true; 
}

void AudioPlaySystem::setSampleParameters(float clockfreq, float samplerate) {
}

void AudioPlaySystem::reset(void) {

}

void AudioPlaySystem::stop(void)
{
	__disable_irq();
	playing = false;	
	__enable_irq();
}

bool AudioPlaySystem::isPlaying(void) 
{ 
   return playing;
}

void AudioPlaySystem::update(void) {
 	audio_block_t *block;

	// only update if we're playing
	if (!playing) return;

	// allocate the audio blocks to transmit
	block = allocate();
	if (block == NULL) return;

  snd_Mixer((short*)block->data, AUDIO_BLOCK_SAMPLES);
  //memset( (void*)block->data, 0, AUDIO_BLOCK_SAMPLES*2);

	transmit(block);
	release(block);
}

void AudioPlaySystem::sound(int C, int F, int V) {
}

void AudioPlaySystem::step(void) {
}
*/