/*********************
 *      DEFINES
 *********************/

#define AUDIO_SAMPLE_8KHZ   8000
#define AUDIO_SAMPLE_16KHZ  16000
#define AUDIO_SAMPLE_32KHZ  32000
#define AUDIO_SAMPLE_44kHZ  44100

/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  audio_init 
 * --------------------
 * 
 * Initialize the I2S peripherla with the configuration set on the file system_configuration.h.
 * You can choose different audio sample rate:
 *  - 8000 KHz
 *  - 16000 KHz
 *  - 32000 KHz
 *  - 44100 KHz (Not recommend, it can produce overflow on the audio buffer)
 * 
 * Arguments:
 *  -sample_rate: Audio sample rate to configure the driver.
 * 
 * Returns: True if the initialization succeed otherwise false.
 * 
 */
bool audio_init(uint32_t sample_rate);

/*
 * Function:  audio_submit 
 * --------------------
 * 
 * Proceess and send the audio samples to the I2S driver.
 * 
 * Arguments:
 *  -stereoAudioBuffer: Pointer to the stereo audio buffer which previously should be filled by the emulator.
 *  -framecount: To achieved sync audio, is necessary to receive the frame which is being display.
 * 
 * Returns: Nothing.
 * 
 */
void audio_submit(short *stereoAudioBuffer, uint32_t frameCount);

/*
 * Function:  audio_terminate 
 * --------------------
 * 
 * Clean the DMA audio buffer and reset the peripheral to avoid noise when the driver
 * is not playing any sound.
 * 
 * Returns: Nothing.
 * 
 */
void audio_terminate();

/*
 * Function:  audio_volume_get 
 * --------------------
 * 
 * Give the volumen level.
 * 
 * Returns: 0-100 integer volumen level.
 * 
 */
uint8_t audio_volume_get();

/*
 * Function:  audio_volume_set 
 * --------------------
 * 
 * Set the volumen level.
 * 
 * Arguments:
 *  -level: Float number from 0 to 1 in decimal increments.
 * 
 * Returns: Nothing.
 * 
 */
void  audio_volume_set(float level);
