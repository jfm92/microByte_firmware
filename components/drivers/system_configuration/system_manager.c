#include "system_manager.h"
#include "string.h"
#include "esp_ota_ops.h"
#include "soc/dport_reg.h"
#include "soc/efuse_periph.h"
#include "esp32/spiram.h"

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