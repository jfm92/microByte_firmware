/*********************
 *      INCLUDES
 *********************/

#include "stdio.h"
#include "stdlib.h"

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"


/**********************
*  STATIC VARIABLES
**********************/

static const char *TAG = "EXTERNAL_APP";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void external_app_init(const char *app_name){

   /* const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    esp_err_t ret = esp_ota_get_state_partition(running, &ota_state);
 
    if(ret == ESP_OK) printf("ESP_OK\r\n");
    else if(ret == ESP_ERR_INVALID_ARG) printf("ESP_ERR_INVALID_ARG\r\n");
    else if(ret == ESP_ERR_NOT_SUPPORTED ) printf("ESP_ERR_NOT_SUPPORTED \r\n");
    else if(ret == ESP_ERR_NOT_FOUND  ) printf("ESP_ERR_NOT_FOUND  \r\n");*/

    ESP_LOGI(TAG, "Starting external App installation");

    esp_err_t err;

    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

   // const esp_partition_t *configured = esp_ota_get_boot_partition();

   /* if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);*/

    update_partition = esp_ota_get_next_update_partition(NULL);
    if(update_partition == NULL){
        ESP_LOGE(TAG, "Failed to get OTA partition");
        return -1;
    }

    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
             update_partition->subtype, update_partition->address);
    
/*
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }*/

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        return -1;
    }

    FILE *fd = NULL;

    char name_aux[256];
    sprintf(name_aux,"/sdcard/apps/%s",app_name);

    fd = fopen(name_aux, "rb");
    if(fd == NULL){
        ESP_LOGE(TAG,"Opening error with: %s",name_aux);
        return -1;
    }

    static char write_buffer[512 + 1] = { 0 };
    int binary_file_length = 0;
    while(1){
        size_t data_read = fread(write_buffer, 1, 512, fd);

        err = esp_ota_write( update_handle, (const void *)write_buffer, data_read);
        if (err != ESP_OK) {
            ESP_LOGE(TAG,"OTA write error");
        }

        binary_file_length += data_read;

        ESP_LOGI(TAG, "Written image length %d", binary_file_length);

        if(data_read < 512) break;
    }

    ESP_LOGI(TAG, "OTA Write correct");

    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
    }


    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));

    }
}