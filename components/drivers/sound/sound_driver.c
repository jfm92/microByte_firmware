#include "sound_driver.h"

#include "freertos/FreeRTOS.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"

// TODO: Save the volume level on the static memory

static float volume_level = 0.8f; // 80% of the sound level

void audio_init(int sample_rate)
{
    printf("%s: sample_rate=%d\n", __func__, sample_rate);

    // NOTE: buffer needs to be adjusted per AUDIO_SAMPLE_RATE
    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX, // Only TX
        .sample_rate = sample_rate,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 512,                       // (416samples * 2ch * 2(short)) = 1664
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, //Interrupt level 1
        .use_apll = false};

    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_pin_config_t pin_config = {
        .bck_io_num = 27,
        .ws_io_num = 26,
        .data_out_num = 25,
        .data_in_num = -1 //Not used
    };
    i2s_set_pin(I2S_NUM, &pin_config);
}

void audio_submit(short *stereoAudioBuffer, int frameCount)
{

    short currentAudioSampleCount = frameCount * 2;

    for (short i = 0; i < currentAudioSampleCount; ++i)
    {
        int sample = stereoAudioBuffer[i] * volume_level;
        if (sample > 32767)
            sample = 32767;
        else if (sample < -32767)
            sample = -32767;

        stereoAudioBuffer[i] = (short)sample;
    }

    int len = currentAudioSampleCount * sizeof(int16_t);
    size_t count;
    i2s_write(I2S_NUM, (const char *)stereoAudioBuffer, len, &count, portMAX_DELAY);
    if (count != len)
    {
        printf("i2s_write_bytes: count (%d) != len (%d)\n", count, len);
        abort();
    }
}

void audio_terminate()
{
    i2s_zero_dma_buffer(I2S_NUM);
    i2s_stop(I2S_NUM);

    i2s_start(I2S_NUM);
}

uint8_t audio_volume_get(){
    return volume_level*100;
}

void audio_volume_set(float level){
    volume_level = level/100.0f;
    printf("volume %f\r\n",volume_level);
}