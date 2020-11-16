/*********************
 *      INCLUDES
 *********************/
#include "stdint.h"
#include "stdbool.h"

#define SDIO    0x00
#define MMC     0x01
#define SDHC    0x02
#define SDSC    0x03

/*********************
 *      DEFINES
 *********************/
struct sd_card_info{
    char card_name[32];
    uint8_t card_type;
    uint16_t card_size;
    uint32_t card_speed;
};

/*********************
 *      EXTERNS
 *********************/
extern struct sd_card_info sd_card_info;

/*********************
 *      FUNCTIONS
 *********************/

/*
 * Function:  sd_init 
 * --------------------
 * 
 * Initialize the SPI bus and set the proper configuration to set-up the FAT file system.
 * 
 * Returns: True if the initialization suceed otherwise false.
 * 
 */
bool sd_init();


/*
 * Function:  sd_game_list 
 * --------------------
 * 
 * This function  gives the number of game available and the name of them for each console.
 * 
 * Arguments:
 *  -game_name: Two dimensions array in which will be save the game names.
 *  -console: Console to check the available games.
 * 
 * Returns: Number of available games for the console.
 * 
 */
uint8_t sd_game_list(char game_name[30][100], uint8_t console);

/*
 * Function:  sd_app_list 
 * --------------------
 * 
 * This function gives the number of update or external applications available on the SD card.
 * 
 * Arguments:
 *  -app_name: Two dimensions array in which will be save the external application name or the updates available on the SD
 *  -update: True is you are looking for an update binary, otherwise false for the external applications.
 * 
 * Returns: Number of available external apps of updates binaries.
 * 
 */
uint8_t sd_app_list(char app_name[30][100], bool update);

/*
 * Function:  sd_file_size 
 * --------------------
 * 
 * Given a file path, it returns the size of the file
 * 
 * Arguments:
 *  -path: Valid path to the file.
 * 
 * Returns: The size of the file in bytes.
 * 
 */
size_t sd_file_size(const char *path);

/*
 * Function:  sd_get_file 
 * --------------------
 * 
 * Given a valid file path, copy a file from the SD card to the RAM memory.
 * 
 * Note: Only valid for files with a size under 3 MBytes.
 * 
 * Arguments:
 *  -path: Valid path to the file.
 *  -data: Pointer to the memory region where the data will be saved.
 * 
 * Returns: Nothing.
 * 
 */
void  sd_get_file (const char *path, void * data);

/*
 * Function:  sd_get_file_flash 
 * --------------------
 * 
 * Given a valid file path, copy a file from the SD card to the "storage" partition on the flash memory.
 * 
 * Note: A valid 4 Mbyte data partition should set previously on the partition.csv file to use this feature.
 * It's not recommendable to use in files under 3 MByte, because the write speed is significantly slower than
 * the memory RAM alternative.
 * 
 * Arguments:
 *  -path: Valid path to the file.
 * 
 * Returns: Pointer to the region of the flash memory where is allocated the data.
 * 
 */
char * IRAM_ATTR sd_get_file_flash (const char *path);

/*
 * Function:  sd_mounted 
 * --------------------
 * 
 * Return if the SD Card and the filesystem was properly mounted.
 * 
 * Returns: True if it was mounted the SD Card
 * 
 */
bool sd_mounted();

/*********WIP**********/
void sd_format();
void sd_unmount();
void sd_mount();