#define I2S_NUM I2S_NUM_0
#define VOLUME_LEVEL_COUNT (5)

void audio_init(int sample_rate);
void audio_submit(short *stereoAudioBuffer, int frameCount);