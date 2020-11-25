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
#include "sd_storage.h"

/*********************
 *      DEFINES
 *********************/
#define MOUNT_POINT     "/sdcard"
#define SPI_DMA_CHAN    2


/**********************
*      VARIABLES
**********************/
bool SD_mount = false;
struct sd_card_info sd_card_info;

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

bool sd_init(){
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

    // Mount Fat filesystem(Only one file at the time)
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
        return false;
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
    if(stat("/sdcard/Game_Gear", &st) == -1){
        ESP_LOGI(TAG, "No Game_Gear folder found, creating it");
        mkdir("/sdcard/Game_Gear", 0700);
        mkdir("/sdcard/Game_Gear/Save_Data", 0700);
    }
    if(stat("/sdcard/apps", &st) == -1){
        ESP_LOGI(TAG, "No apps folder found, creating it");
        mkdir("/sdcard/apps", 0700);
    }

    SD_mount = true;

    // Get data from the SD card
    sd_card_info.card_size = ((uint64_t) card->csd.capacity) * card->csd.sector_size / (1024 * 1024);
    strcpy(sd_card_info.card_name,card->cid.name);
    sd_card_info.card_speed = card->max_freq_khz;

    if(card->is_sdio) sd_card_info.card_type        = SDIO;
    else if(card->is_mmc) sd_card_info.card_type    = MMC;
    else
    {
        sd_card_info.card_type = SDHC;
    }
    

    ESP_LOGI(TAG,"SD Card detected:\n -Name: %s\n -Capacity: %i MB\n -Speed: %i Khz\n -Type: %i\n" \
    ,sd_card_info.card_name,sd_card_info.card_size,sd_card_info.card_speed,sd_card_info.card_type);

    return true;
}


uint8_t sd_game_list(char game_name[30][100],uint8_t console){
    
    struct dirent *entry;
    //TODO: Improve game name management, at this moment the max size of the name is 100 characters and 30 games per console
    // Open the folder of the specific console
    DIR *dir = NULL;
    if(console == NES) dir =  opendir("/sdcard/NES");
    else if(console == GAMEBOY) dir =  opendir("/sdcard/GameBoy");
    else if(console == GAMEBOY_COLOR) dir =  opendir("/sdcard/GameBoy_Color");
    else if(console == SNES) dir =  opendir("/sdcard/SNES");
    else if(console == SMS) dir =  opendir("/sdcard/Master_System");
    else if(console == GG) dir =  opendir("/sdcard/Game_Gear");


    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : 0x%02x", console);
        return 0;
    }
    
    //Loop to find the game of each console base on the file extension
    uint8_t i =0;
    while((entry = readdir(dir)) != NULL){
 
        size_t nameLength = strlen(entry->d_name);

        // TODO: Rework game list maker, set by alphabetical order

        if ((strcmp(entry->d_name + (nameLength - 4), ".nes") == 0) && console == NES) {
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }
        else if ((strcmp(entry->d_name + (nameLength - 3), ".gb") == 0) && console == GAMEBOY){
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }
        else if ((strcmp(entry->d_name + (nameLength - 4), ".gbc") == 0) && console == GAMEBOY_COLOR){
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }
        else if ((strcmp(entry->d_name + (nameLength - 4), ".sms") == 0) && console == SMS){
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }
        else if ((strcmp(entry->d_name + (nameLength - 3), ".gg") == 0) && console == GG){
            sprintf(game_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",game_name[i]);
            i++;
        }
    }
    // Return the number of files
    return i;
}

uint8_t sd_app_list(char app_name[30][100],bool update){
    struct dirent *entry;

    DIR *dir = NULL;
    if(update) dir =  opendir("/sdcard");
    else dir =  opendir("/sdcard/apps");
    
    // Only find .bin file on the apps folder
    uint8_t i =0;
    while((entry = readdir(dir)) != NULL){
        
        size_t nameLength = strlen(entry->d_name);

        if(strcmp(entry->d_name + (nameLength - 4), ".bin") == 0) {
            sprintf(app_name[i],"%s",entry->d_name);
            ESP_LOGI(TAG, "Found %s ",app_name[i]);
            i++;
        }
    }

    return i;
}
    

size_t sd_file_size(const char *path){
    
    FILE *fd = fopen(path, "rb");

    fseek(fd, 0, SEEK_END);
    size_t actual_size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    ESP_LOGI(TAG,"Size: %i bytes",actual_size);
    fclose(fd);

    return actual_size;
}

void sd_get_file (const char *path, void * data){
    const size_t BLOCK_SIZE = 512;// We're going to read file in chunk of 512 Bytes
    size_t r = 0;

    FILE *fd = fopen(path, "rb"); //Open the file in binary read mode

    if(fd==NULL){
       ESP_LOGE(TAG, "Error opening: %s ",path);
    }
    while (true){
        //__asm__("memw"); // Protect the write into the RAM memory
        size_t count = fread((uint8_t *)data + r, 1, BLOCK_SIZE, fd);
       // __asm__("memw");

        r += count;
        if (count < BLOCK_SIZE) break;
    }

    fclose(fd);
}


char * IRAM_ATTR sd_get_file_flash (const char *path){
     const size_t BLOCK_SIZE = 4096*2;
    char *map_ptr;// Pointer to file in the internal flash.
    
    spi_flash_mmap_handle_t map_handle;

    const esp_partition_t *partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "data_0");
    if(partition == NULL){
        ESP_LOGE(TAG, "Partition NULL");
    }
    ESP_LOGI(TAG,"Partition label %s, ffset 0x%x with size 0x%x\r\n",partition->label,partition->address, partition->size);
    

    ESP_ERROR_CHECK(esp_partition_erase_range(partition, 0, partition->size));

    size_t r = 0;

    FILE *fd = fopen(path, "rb"); //Open the file in binary read mode

    if(fd==NULL){
       ESP_LOGE(TAG, "Error opening: %s ",path);
    }

    char * temp_buffer;
    temp_buffer =  malloc(BLOCK_SIZE);
    esp_fill_random(temp_buffer,BLOCK_SIZE);

    while (true){
        __asm__("memw"); // Protect the write into the RAM memory
        size_t count = fread(temp_buffer, 1, BLOCK_SIZE, fd);
        esp_partition_write(partition, r, temp_buffer, BLOCK_SIZE);
        __asm__("memw");

        r += count;
        if (count < BLOCK_SIZE) break;
    }

    close(fd);
    free(temp_buffer);
    // Return a pointer to the position of the saved file on the internal flash.

	ESP_ERROR_CHECK(esp_partition_mmap(partition, 0, partition->size, SPI_FLASH_MMAP_DATA, &map_ptr, &map_handle));
   return map_ptr;
}


bool sd_mounted(){
    return SD_mount;
}



