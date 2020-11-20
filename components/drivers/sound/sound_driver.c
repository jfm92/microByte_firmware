/*********************
 *      INCLUDES
 *********************/

#include "freertos/FreeRTOS.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2s.h"
#include "driver/rtc_io.h"

#include "sound_driver.h"
#include "system_configuration.h"

/**********************
*      VARIABLES
**********************/
float volume_level = 0.8f; // When starts the sound level it's at the 80%

/**********************
*  STATIC VARIABLES
**********************/
static const char *TAG = "SOUND_DRIVER";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

bool audio_init(uint32_t sample_rate){
    ESP_LOGI(TAG,"Audio configuration init.");

    ESP_LOGI(TAG,"Audio Sample Rate: %i",sample_rate);

    i2s_config_t i2s_config = {
        .mode = I2S_MODE_MASTER | I2S_MODE_TX, // Only TX
        .sample_rate = sample_rate,
        .bits_per_sample = 16,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, //2-channels
        .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
        .dma_buf_count = 6,
        .dma_buf_len = 512,
        .intr_alloc_flags = 0, 
        .use_apll = false};

    if(i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL) != ESP_OK){
        ESP_LOGE(TAG,"I2S driver error install error");
        return false;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_DATA_O,
        .data_in_num = -1 //Not used
    };

    if(i2s_set_pin(I2S_NUM, &pin_config) != ESP_OK){
        ESP_LOGE(TAG,"I2S set pin error.");
        return false;
    }

    return true;
}

void audio_submit(short *stereoAudioBuffer, uint32_t frameCount){

    uint32_t audio_length = frameCount * 2 * sizeof(int16_t);
    size_t count;

    // Normalize the size of the sample to avoid size bigger than +- 32767
    for(short i = 0; i < frameCount * 2; ++i){
        int sample = stereoAudioBuffer[i] * volume_level; // Set the volumen level to the sample

        if (sample > 32767) sample = 32767;
        else if (sample < -32767) sample = -32767;

        stereoAudioBuffer[i] = (short)sample;
    }

    i2s_write(I2S_NUM, (const char *)stereoAudioBuffer, audio_length, &count, portMAX_DELAY);

    if(count != audio_length){
        ESP_LOGE(TAG,"I2S Write error:\n Send count: %d\n Audio_Length: %d",count,audio_length);
        abort();
    }
}

void audio_terminate(){
    i2s_zero_dma_buffer(I2S_NUM); // Clean the DMA buffer
    i2s_stop(I2S_NUM);
    i2s_start(I2S_NUM);
}

uint8_t audio_volume_get(){
    ESP_LOGI(TAG,"Volumen level: %i",(uint8_t)volume_level*100);
    return volume_level*100;
}

void audio_volume_set(float level){
    if(level >= 0 || level <= 100) volume_level = level/100.0f;
    ESP_LOGI(TAG,"Volumen level set: %i",(uint8_t)volume_level *100);
}