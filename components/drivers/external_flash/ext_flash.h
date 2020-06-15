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
uint8_t ext_flash_ussage(void);
uint8_t ext_flash_game_list(char * game_name);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*EXT_FLASH_H*/