/*********************
 *      INCLUDES
 *********************/

#include "stdio.h"
#include "stdlib.h"
#include <string.h>

#include "freertos/FreeRTOS.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_ota_ops.h"

/**********************
*  STATIC VARIABLES
**********************/

static const char *TAG = "FW_UPDATE";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void update_check(){
    esp_ota_img_states_t ota_state;
    const esp_partition_t * running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            //If all the hardware initialize the firmware should be fine, so we mark as a valid one
            esp_ota_mark_app_valid_cancel_rollback();
            esp_ota_erase_last_boot_app_partition();
            ESP_LOGE(TAG, "Update succed!");
        }
    }
}

int update_init(char *fw_name){

    ESP_LOGI(TAG,"Firmware update process");

    esp_err_t err;
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;
    const esp_partition_t *running = NULL;
    esp_app_desc_t running_fw;
    esp_app_desc_t update_fw;
    esp_app_desc_t invalid_fw;

    static char temp_buffer[1024];
    uint32_t update_size = 0;


    // Get app data information to compare with the future version.
    running = esp_ota_get_running_partition();
    err = esp_ota_get_partition_description(running, &running_fw);
    if(err != ESP_OK){
        ESP_LOGE(TAG, "Error getting running firmware information.");
        return -1;
    }

    ESP_LOGI(TAG, "Running firmware version: %s", running_fw.version);

    
    // We're on the factory partition, get the OTA partition
    update_partition = esp_ota_get_next_update_partition(NULL);
    if(update_partition == NULL){
        ESP_LOGE(TAG, "Failed to get OTA partition");
        return -1;
    }


    ESP_LOGI(TAG,"Found available partition subtype %d at offset 0x%x with label %s" \
                                                        ,update_partition->subtype \
                                                        ,update_partition->address \
                                                        ,update_partition->label);

    
    // Get app data information of the last failed update
    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
    esp_app_desc_t invalid_app_info;
    if (esp_ota_get_partition_description(last_invalid_app, &invalid_fw) == ESP_OK) {
        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_fw.version);
    }


    //Open the new fw file
    char name_aux[256];
    sprintf(name_aux,"/sdcard/%s",fw_name);
    FILE *fd = fopen(name_aux, "rb");
    if(fd == NULL){
        ESP_LOGE(TAG,"Opening error with: %s",name_aux);
        return -1;
    }

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
        return -1;
    }

    // Check the header to know if it's a valid version of the firmware
    size_t data_header = fread(temp_buffer, 1, 1024, fd);

    if(data_header > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)){
        memcpy(&update_fw, &temp_buffer[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));

        // Check if you're triying to flash the same firmware
        if(!memcmp(update_fw.version,running_fw.version,sizeof(running_fw.version))){
            //Not sure if i should stop the update if it's the same version?
            ESP_LOGE(TAG, "Same firmware version.");
            //return -1;
        }

        if(last_invalid_app != NULL){
            if (!memcmp(invalid_app_info.version, update_fw.version, sizeof(update_fw.version))) {
                ESP_LOGE(TAG,"New firmware is the same previous invalid version");
                return -1;
            }
        }
    }

    fseek(fd, 0, SEEK_SET);
    memset(temp_buffer,0,sizeof(temp_buffer));

    // The update is fine, so we will copy the new FW from the SD card to the OTA partition
    while(1){
        size_t data_read = fread(temp_buffer, 1, 1024, fd);

        err = esp_ota_write( update_handle, (const void *)temp_buffer, data_read);
        if (err != ESP_OK) {
            ESP_LOGE(TAG,"OTA write error");
            return -1;
        }


        update_size += data_read;
        ESP_LOGI(TAG, "Written to OTA: %d",update_size);

        if(data_read < 1024) break;

    }

    fclose(fd);

    ESP_LOGI(TAG, "Update process succed!");

    //Close OTA and check if the files are fine.
    err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            return -1;
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        return -1;
    }

    ESP_LOGI(TAG, "Changing boot partition to OTA");
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));

    }   

    return 1;
}

