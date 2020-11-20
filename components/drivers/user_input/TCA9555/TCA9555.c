/*********************
*      INCLUDES
*********************/
#include "stdbool.h"

#include "esp_log.h"
#include "driver/i2c.h"

#include "system_configuration.h"

/*********************
*      DEFINES
*********************/

#define CONFIG_REG 0x06
#define READ_REG 0x00 


/**********************
*  STATIC PROTOTYPES
**********************/

static const char *TAG = "TCA9555";

static int8_t TCA955_I2C_write(uint8_t I2C_bus, uint8_t *data, size_t size);
static int8_t TCA955_I2C_read(uint8_t I2C_bus, uint8_t *data, size_t size);

/**********************
*   GLOBAL FUNCTIONS
**********************/

bool TCA955_init(void){
    //Init I2C bus as master
    esp_err_t ret;

    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = MUX_SDA;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = MUX_SCL;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_CLK ;

    i2c_param_config(0, &conf);

    ret =  i2c_driver_install(0, conf.mode,0,0, 0);
    
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "I2C driver initialization fail");
        return false;
    }

    //Check if the device is connected
    if(TCA955_I2C_write(0,0x00,1) == -1){
        ESP_LOGE(TAG,"TCA9555 not detected");
        return false;
    }

    ESP_LOGI(TAG,"TCA9555 detected");

    return true;
}

int16_t TCA9555_readInputs(void){
    uint8_t data[2] = {0xFF,0xFF};

    TCA955_I2C_write(0, READ_REG, 1);
    TCA955_I2C_read(0, data, 2);

    uint16_t data_out = ((uint16_t)data[1] << 8) | data[0];

    return data_out;
}

bool TCA9555_pinMode(uint8_t pin){
    uint16_t pinMode = 0xFFFF;

    if(pin > 16){
        ESP_LOGE(TAG, "Pin out of the range (0-16)");
        return -1;
    }

    pinMode = pinMode | (0x01 << pin);

    uint8_t pinMode_h = (uint8_t) (pinMode >> 8);
    uint8_t pinMode_l = (uint8_t) (pinMode & 0X00FF);
    uint8_t data[3] = {CONFIG_REG,pinMode_l,pinMode_h};

    if(TCA955_I2C_write(0, data, 3) == -1 ){
        ESP_LOGE(TAG, "Error setting pinMode");
        return false;
    }
   
    return true;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int8_t TCA955_I2C_write(uint8_t I2C_bus, uint8_t *data, size_t size){

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, ( I2C_dev_address  << 1 ) | I2C_MASTER_WRITE, 0x1);
    i2c_master_write(cmd, &data, size, 0x1); // Using &data will give a warning a compilation time, but is necessary to avoid I2C invalid address at running time
    i2c_master_stop(cmd);

    esp_err_t ret = i2c_master_cmd_begin(0, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if(ret != ESP_OK){
        ESP_LOGE(TAG, "Write error err = %d",ret );
        return -1;
    }
    
    return 1;

}

static int8_t TCA955_I2C_read(uint8_t I2C_bus, uint8_t *data, size_t size){

   if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_dev_address << 1) | I2C_MASTER_READ, 0x1);
    if (size > 1) {
        i2c_master_read(cmd, data, size - 1, 0x0);
    }
    i2c_master_read_byte(cmd, data + size - 1, 0x1);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_bus, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret; 
}