/*********************
 *      INCLUDES
 *********************/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "scr_spi.h"
#include "system_configuration.h"

#include <string.h>


/**********************
*  STATIC PROTOTYPES
**********************/
static spi_device_handle_t spi;
static void scr_spi_pre_transfer_callback(spi_transaction_t *t);

/**********************
*   GLOBAL FUNCTIONS
**********************/
void scr_spi_init(void){
    //GPIO initialization
    gpio_set_direction(HSPI_DC, GPIO_MODE_OUTPUT);
    gpio_set_direction(HSPI_RST, GPIO_MODE_OUTPUT);

    //SPI bus configuration
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = HSPI_MOSI,
        .sclk_io_num = HSPI_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 153600};
    
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = HSPI_CLK_SPEED,
        .mode = 2,                   //SPI mode 2
        .spics_io_num = -1, //CS pin
        .queue_size = 1,
        .pre_cb = scr_spi_pre_transfer_callback,
        .post_cb = NULL,
        //.flags = SPI_DEVICE_HALFDUPLEX
    };

    // SPI bus initialization
    esp_err_t ret;
    ret = spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret == ESP_OK);

    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(HSPI_HOST, &devcfg, &spi);
    assert(ret == ESP_OK);
}

void scr_spi_send(uint8_t *data, uint16_t length, int dc){
    if (length == 0)
        return; //no need to send anything

    spi_transaction_ext_t t = {0};
   
    t.base.length = length * 8;    //Length is in bytes, transaction length is in bits.
    t.base.tx_buffer = data;       //Data
    t.base.user = (void *)dc;
  
    static spi_transaction_ext_t queuedt;
    memcpy(&queuedt, &t, sizeof t);
    spi_device_queue_trans(spi, (spi_transaction_t *) &queuedt, portMAX_DELAY);
    

    spi_transaction_t *rt;
    spi_device_get_trans_result(spi, &rt, portMAX_DELAY);
}


void scr_spi_send_lines(int ypos, int xpos, int width, uint16_t *linedata, int lineCount){
    esp_err_t ret;
    int x;
    static spi_transaction_t trans[6];

    for (x = 0; x < 6; x++)
    {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0)
        {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void *)0;
        }
        else
        {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void *)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;                      //Column Address Set
    trans[1].tx_data[0] = xpos >> 8;                 //Start Col High
    trans[1].tx_data[1] = xpos & 0xff;               //Start Col Low
    trans[1].tx_data[2] = (width + xpos - 1) >> 8;   //End Col High
    trans[1].tx_data[3] = (width + xpos - 1) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;                      //Page address set
    trans[3].tx_data[0] = ypos >> 8;                 //Start page high
    trans[3].tx_data[1] = ypos & 0xff;               //start page low
    trans[3].tx_data[2] = (ypos + lineCount) >> 8;   //end page high
    trans[3].tx_data[3] = (ypos + lineCount) & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;                      //memory write
    trans[5].tx_buffer = linedata;                   //finally send the line data
    trans[5].length = width * 2 * 8 * lineCount;     //Data length, in bits
    trans[5].flags = 0;                              //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++)
    {
        ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }
}

void scr_spi_send_color(const uint8_t *data, uint16_t length,uint64_t addr){

}

/**********************
*   STATIC FUNCTIONS
**********************/

void scr_spi_rst(void){
    gpio_set_level(HSPI_RST,0);
    vTaskDelay(100 / portTICK_RATE_MS);
    gpio_set_level(HSPI_RST,1);
}

static void scr_spi_pre_transfer_callback(spi_transaction_t *t){
    int dc = (int)t->user;
    gpio_set_level(HSPI_DC, dc);
}




