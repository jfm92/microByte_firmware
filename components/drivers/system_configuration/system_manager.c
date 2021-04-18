/*********************
 *      INCLUDES
 *********************/
#include "system_manager.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "soc/dport_reg.h"
#include "soc/efuse_periph.h"
#include "esp32/spiram.h"

#include "nvs_flash.h"
#include "nvs.h"


nvs_handle_t config_handle;


/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int system_memory(uint8_t memory){

    multi_heap_info_t info; 

    if(memory == MEMORY_DMA) heap_caps_get_info(&info, MALLOC_CAP_DMA);
    else if(memory == MEMORY_INTERNAL) heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    else if(memory == MEMORY_SPIRAM) heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    else if(memory == MEMORY_ALL)  heap_caps_get_info(&info, MALLOC_CAP_DEFAULT);
    else{
        return -1;
    }

    return info.total_free_bytes;
}

void system_info(){
    const esp_app_desc_t *app_desc = esp_ota_get_app_description();

    // Get App version
    strcpy(app_version,app_desc->version);

    // Get CPU
    uint32_t chip_ver = REG_GET_FIELD(EFUSE_BLK0_RDATA3_REG, EFUSE_RD_CHIP_VER_PKG);
    uint32_t pkg_ver = chip_ver & 0x7;

    if (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32D2WDQ5) strcpy(cpu_version,"ESP32-D2WD");
    else if ((pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD2) || (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32PICOD4)) strcpy(cpu_version,"ESP32-PICO");
    else if ((pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ6) || (pkg_ver == EFUSE_RD_CHIP_VER_PKG_ESP32D0WDQ5)) strcpy(cpu_version,"ESP32-D0WD");
    else{
       strcpy(cpu_version,"UNKWON"); 
    }

    // Get RAM
    RAM_size = esp_spiram_get_size()/(1024*1024);

    // Get Flash
    esp_flash_get_size(NULL, &FLASH_size);
    FLASH_size = FLASH_size/(1024*1024);
}

void system_init_config(){
    //TODO: Implement error control if it fails initialization.
    nvs_flash_init();
}

//set save reset reason
void system_set_state(int8_t state){
    //TODO: Check if it's a valid code
    nvs_open("nvs", NVS_READWRITE, &config_handle);
    nvs_set_i8(config_handle, "prev_state", state);
    nvs_close(&config_handle);
}
    
// get reset reason
int8_t system_get_state(){
    int8_t prev_state;
    nvs_open("nvs", NVS_READWRITE, &config_handle);
    nvs_get_i8(config_handle, "prev_state", &prev_state);
    nvs_close(&config_handle);
    return prev_state;
}

void system_save_config(uint8_t config, int8_t value){
    nvs_open("nvs", NVS_READWRITE, &config_handle);
    if(config == SYS_BRIGHT && value <= 100){
        nvs_set_i8(config_handle, "scr_bright", value);
    }
    else if(config == SYS_VOLUME && value <= 100){
        nvs_set_i8(config_handle, "sound_volume", value);
    }
    else if(config == SYS_GUI_COLOR){
        nvs_set_i8(config_handle, "GUI_color", value);
    }
    else if(config == SYS_STATE_SAV_BTN){
        nvs_set_i8(config_handle, "Save_State", value);
        printf("Set save_State value %i\r\n",value);
    }
    nvs_close(&config_handle);
}

int8_t system_get_config(uint8_t config){
    nvs_open("nvs", NVS_READWRITE, &config_handle);
    int8_t value = -1;
    if(config == SYS_BRIGHT && value <= 100){
        nvs_get_i8(config_handle, "scr_bright", &value);
        if(value < 1 || value > 100) value = 100;
    }
    else if(config == SYS_VOLUME && value <= 100){
        nvs_get_i8(config_handle, "sound_volume", &value);
        if(value < 0 || value > 100) value = 80; //Default value of 80%
    }
    else if(config == SYS_GUI_COLOR){
        nvs_get_i8(config_handle, "GUI_color", &value);
        //if(value != 0 || value != 1) value = 0;
    }
    else if(config == SYS_STATE_SAV_BTN) {
        nvs_get_i8(config_handle, "Save_State", &value);
        printf("Value get %i\r\n",value);
    }
    nvs_close(&config_handle);

    return value;
}

