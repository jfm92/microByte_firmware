#include "stdint.h"

#define I2S_NUM I2S_NUM_0
#define VOLUME_LEVEL_COUNT (5)

void audio_init(int sample_rate);
void audio_submit(short *stereoAudioBuffer, int frameCount);
void audio_terminate();
uint8_t audio_volume_get();
void  audio_volume_set(float level);
