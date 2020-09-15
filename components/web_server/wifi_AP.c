#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "wifi_AP.h"

static const char *TAG = "Wi-Fi AP";

int8_t wifi_AP_init(){
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }

    tcpip_adapter_init();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    if(esp_wifi_init(&cfg) != ESP_OK){
        ESP_LOGE(TAG,"Failed to initialize Wi-Fi AP");
        return -1;
    }

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = WIFI_AP_NAME,
            .ssid_len = strlen(WIFI_AP_NAME),
            .password = WIFI_AP_PASS,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    if(esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ){
        ESP_LOGE(TAG,"Failed to set AP mode");
        return -1;
    }

    if(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config) != ESP_OK){
        ESP_LOGE(TAG,"Failed to set configuration");
        return -1;
    }

    if(esp_wifi_start() != ESP_OK){
        ESP_LOGE(TAG,"Failed to start Wi-Fi");
        return -1;
    }

    ESP_LOGI(TAG, "Wi-Fi AP initialize with SSID: %s, PASSWORD: %s",WIFI_AP_NAME,WIFI_AP_PASS);

    return 0;
}

void wifi_AP_deinit(){
    nvs_flash_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
}