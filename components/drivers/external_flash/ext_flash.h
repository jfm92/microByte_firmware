/**
 * @file ext_flash.h
 *
 */

#ifndef DISP_SPI_H
#define DISP_SPI_H

#ifdef __cplusplus
extern "C" {
#endif

/*********************
*      INCLUDES
*********************/


/*********************
*      DEFINES
*********************/

/**********************
*      TYPEDEFS
**********************/

/**********************
* GLOBAL PROTOTYPES
**********************/

void ext_flash_init(void);
int ext_flash_ussage(void);
uint8_t ext_flash_game_list(char * game_name);
void ext_flash_mount_fs(void);
void ext_flash_unmount_fs(void);
char * IRAM_ATTR ext_flash_get_file (const char *path);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*EXT_FLASH_H*/