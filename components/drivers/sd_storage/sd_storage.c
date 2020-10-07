/*********************
 *      INCLUDES
 *********************/

#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <errno.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"

#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#include "system_configuration.h"
#include "system_manager.h"

/*********************
 *      DEFINES
 *********************/
#define MOUNT_POINT     "/sdcard"
#define SPI_DMA_CHAN    2


/**********************
*      VARIABLES
**********************/
bool SD_mount = false;

/**********************
*  STATIC VARIABLES
**********************/
static const char *TAG = "SD_CARD";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

uint8_t sd_init(){
    ESP_LOGI(TAG, "Init SD Card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso   =     VSPI_MISO;
    slot_config.gpio_mosi   =     VSPI_MOSI;
    slot_config.gpio_sck    =     VSPI_CLK;
    slot_config.gpio_cs     =     VSPI_CS0;
    slot_config.dma_channel =     SPI_DMA_CHAN;
    host.slot               =     VSPI_HOST;
    host.max_freq_khz       =     SDMMC_FREQ_DEFAULT;

    // Mount Fat filesystem, it only can open one file at the same time
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 1,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card.");
        }
        return -1;
    }

    // Check if the folder tree exists. If not create it.
    struct stat st;
    if(stat("/sdcard/NES", &st) == -1){
        ESP_LOGI(TAG, "No NES folder found, creating it");
        mkdir("/sdcard/NES", 0700);
        mkdir("/sdcard/NES/Save_Data", 0700);
    }
    if(stat("/sdcard/GameBoy_Color", &st) == -1){
        ESP_LOGI(TAG, "No GameBoy Color folder found, creating it");
        mkdir("/sdcard/GameBoy_Color", 0700);
        mkdir("/sdcard/GameBoy_Color/Save_Data", 0700);
    }
    if(stat("/sdcard/GameBoy", &st) == -1){
        ESP_LOGI(TAG, "No GameBoy folder found, creating it");
        mkdir("/sdcard/GameBoy", 0700);
        mkdir("/sdcard/GameBoy/Save_Data", 0700);
    }
    if(stat("/sdcard/SNES", &st) == -1){
        ESP_LOGI(TAG, "No SNES folder found, creating it");
        mkdir("/sdcard/SNES", 0700);
        mkdir("/sdcard/SNES/Save_Data", 0700);
    }
    if(stat("/sdcard/Master_System", &st) == -1){
        ESP_LOGI(TAG, "No Master_System folder found, creating it");
        mkdir("/sdcard/Master_System", 0700);
        mkdir("/sdcard/Master_System/Save_Data", 0700);
    }

    SD_mount = true;
    return 1;
}


uint8_t sd_game_list(char game_name[30][100],uint8_t console){
    
    struct dirent *entry;

    DIR *dir = NULL;
    if(console == NES) dir =  opendir("/sdcard/NES");
    else if(console == GAMEBOY) dir =  opendir("/sdcard/GameBoy");
    else if(console == GAMEBOY_COLOR) dir =  opendir("/sdcard/GameBoy_Color");
    else if(console == SNES) dir =  opendir("/sdcard/SNES");
    else if(console == SMS) dir =  opendir("/sdcard/Master_System");

    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : 0x%02x", console);
        return 0;
    }
    // Save the name of all the files available on the external flash
    uint8_t i =0;
    while((entry = readdir(dir)) != NULL){
        // List everyfile except the save folder
        if(strcmp(entry->d_name,"Save_Data")!=0){
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }

    }
    // Return the number of files
    return i;
}


void  IRAM_ATTR  sd_get_file (const char *path, void * data){
    const size_t BLOCK_SIZE = 512;// We're going to read file in chunk of 512 Bytes
    size_t r = 0;

    FILE *fd = fopen(path, "rb"); //Open the file in binary read mode

    if(fd==NULL){
       ESP_LOGE(TAG, "Error opening: %s ",path);
    }
    while (true){
        __asm__("memw"); // Protect the write into the RAM memory
        size_t count = fread((uint8_t *)data + r, 1, BLOCK_SIZE, fd);
        __asm__("memw");

        r += count;
        if (count < BLOCK_SIZE) break;
    }

}


bool sd_mounted(){
    return SD_mount;
}

