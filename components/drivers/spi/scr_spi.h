#ifndef SCR_SPI_H
#define SCR_SPI_H

#ifdef __cplusplus
extern "C"
{
#endif

/*********************
 *      INCLUDES
 *********************/
#include <stdint.h>

/*********************
 *      DEFINES
 *********************/
#define CMD_ON 0
#define DATA_ON 1

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void scr_spi_init(void);
void scr_spi_send(uint8_t *data, uint16_t length, int dc);
void scr_spi_send_lines(int ypos, int xpos, int width, uint16_t *linedata, int lineCount);
void scr_spi_rst(void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /*SRC_SPI_H*/
