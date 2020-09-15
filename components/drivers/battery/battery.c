
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include <driver/adc.h>
#include "esp_log.h"
#include "esp_adc_cal.h"

#include "battery.h"
#include "system_manager.h"

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_0;   
static const adc_atten_t atten = ADC_ATTEN_11db; //1dB of attenuation to obtain the full voltage range of the GPIO (Up to 3.9V)
static const adc_unit_t unit = ADC_UNIT_1;

static const char *TAG = "Battery";

void battery_init(void){
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(channel,atten);

    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, 1100, adc_chars);
    
}

void batteryTask(void *arg){
    struct BATTERY_STATUS battery_status;

    while(1){

        uint32_t adc_reading = 0;

        //Perform a multisampling reading of the ADC1 Channel 0
        for(int i = 0; i < 128; i++){
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        }

        adc_reading /= 128;

        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        battery_status.voltage = voltage;
        battery_status.percentage = (voltage-2283)/((3200-2283)/100); //TODO: Modify calculus


        if( xQueueSend( batteryQueue,&battery_status, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG,"Battery queue send fail");
        }

        if(battery_status.percentage>=5){
            //Send battery alert if the level is below 5%
            struct SYSTEM_MODE management;

            management.mode = MODE_BATTERY_ALERT;

            if( xQueueSend( modeQueue,&management, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"Mode queue send fail, battery alert!");
            }
        }

        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}