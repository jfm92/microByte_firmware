/*********************
 *      INCLUDES
 *********************/

#include <sys/param.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "esp_partition.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "nvs_flash.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ext_flash.h"

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
 static void vspi_configuration(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;
static const char *TAG = "External_Flash";

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void ext_flash_init(){

    /****************SPI Bus configuration*****************/

    /*We're going to use the W25Q128 external flash memory
    with only the basic pins*/
     const spi_bus_config_t bus_config = {
        .mosi_io_num = VSPI_IOMUX_PIN_NUM_MOSI,
        .miso_io_num = VSPI_IOMUX_PIN_NUM_MISO,
        .sclk_io_num = VSPI_IOMUX_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    /* The HSPI bus is attached to the screen*/

    const esp_flash_spi_device_config_t device_config = {
        .host_id = VSPI_HOST,
        .cs_id = 0,
        .cs_io_num = VSPI_IOMUX_PIN_NUM_CS,
        .io_mode = SPI_FLASH_DIO,
        .speed = ESP_FLASH_40MHZ
    };

    ESP_LOGI(TAG, "Initializing external SPI Flash");
    ESP_LOGI(TAG, "Pin assignments:");
    ESP_LOGI(TAG, "MOSI: %2d   MISO: %2d   SCLK: %2d   CS: %2d",
        bus_config.mosi_io_num, bus_config.miso_io_num,
        bus_config.sclk_io_num, device_config.cs_io_num
    );
    
    /* Initialize the SPI bus on DMA channel 2*/
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &bus_config, 2));

    /****************External flash init*****************/

    /* Attach the external flash to the SPI bus*/
    esp_flash_t* ext_flash;
    ESP_ERROR_CHECK(spi_bus_add_flash_device(&ext_flash, &device_config));

    /* Probe the Flash chip and initialize it*/
    esp_err_t err = esp_flash_init(ext_flash);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize external Flash: %s (0x%x)", esp_err_to_name(err), err);
        return NULL;
    }

    /*Print the ID of the chip*/
    uint32_t id;
    ESP_ERROR_CHECK(esp_flash_read_id(ext_flash, &id));
    ESP_LOGI(TAG, "Initialized external Flash, size=%d KB, ID=0x%x", ext_flash->size / 1024, id);


    /****************Add partition to the external memory*****************/

    const char partition_label = "ext_storage";

   printf("Adding external Flash as a partition, size=%d KB", ext_flash->size / 1024);
    const esp_partition_t* fat_partition;
    esp_partition_register_external(ext_flash, 0, ext_flash->size, "ext_storage", ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, &fat_partition);

    

    /****************Mount partition with FatFS*****************/
       ESP_LOGI(TAG, "Listing data partitions:");
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);

    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "- partition '%s', subtype %d, offset 0x%x, size %d kB",
        part->label, part->subtype, part->address, part->size / 1024);
    }

    esp_partition_iterator_release(it);

    ESP_LOGI(TAG, "Mounting FAT filesystem");
    const esp_vfs_fat_mount_config_t mount_config = {
            .max_files = 16,
            .format_if_mount_failed = true,
            .allocation_unit_size = CONFIG_WL_SECTOR_SIZE
    };
    esp_vfs_fat_spiflash_mount("/ext_flash", "ext_storage", &mount_config, &s_wl_handle);

}

uint8_t ext_flash_game_list(char * game_name){
    
    struct dirent *entry;
    
    //Open the external flash as a directory
    DIR *dir = opendir("/ext_flash/");
    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : %s", "/ext_flash/");
        return 0;
    }
    // Save the name of all the files available on the external flash
    uint8_t i =0;
    while((entry = readdir(dir)) != NULL){
        sprintf(&game_name[i],"%s",entry->d_name);
        ESP_LOGI(TAG, "Found %s ",entry->d_name);

        i++;
    }
    // Return the number of files obtained
    return i;
}

uint8_t ext_flash_ussage(){
    FATFS *fs;
    size_t free_clusters;
    int res = f_getfree("0:", &free_clusters, &fs);
    assert(res == FR_OK);
    size_t total_sectors = (fs->n_fatent - 2) * fs->csize;
    size_t free_sectors = free_clusters * fs->csize;

    // assuming the total size is < 4GiB, should be true for SPI Flash
    size_t bytes_total, bytes_free;
        bytes_total = total_sectors * fs->ssize;
    
        bytes_free = free_sectors * fs->ssize;
    
    ESP_LOGI(TAG, "FAT FS: %d kB total, %d kB free", bytes_total / 1024, bytes_free / 1024);

    //Calcule the percentage
    uint8_t percentage = 100-(100*bytes_free)/bytes_total;
    if(percentage==0) percentage=1;
    return percentage;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
