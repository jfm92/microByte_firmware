/*********************
 *      LIBRARIES
 *********************/
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "lvgl.h"

#include "GUI.h"
#include "system_manager.h"
#include "sd_storage.h"
#include "user_input.h"
#include "sound_driver.h"
#include "st7789.h"

/*********************
 *   ICONS IMAGES
 *********************/

/* Emulator lib icons */
#include "icons/emulators_icon.h"
#include "icons/gameboycolor_icon.h"
#include "icons/gameboy_icon.h"
#include "icons/nes_icon.h"
#include "icons/snes_icon.h"
#include "icons/sega_icon.h"
#include "icons/nogame_icon.h"

/* Wi-Fi lib manager icons */
#include "icons/wifi_lib_icon.h"

/* Bluetooth controller icons */
#include "icons/bt_controller_icon.h"

/* Configuration icons */
#include "icons/settings_icon.h"

/*********************
 *      DEFINES
 *********************/


/**********************
 *  STATIC PROTOTYPES
 **********************/

// Tasks
static bool user_input_task(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);
static void battery_status_task(lv_task_t * task);

// Emulators menu
static void emulators_menu(lv_obj_t * parent);
static void emulators_list_cb(lv_obj_t * parent, lv_event_t e);
static void emulator_list_event(lv_obj_t * parent, lv_event_t e);
static void game_execute_cb(lv_obj_t * parent, lv_event_t e);
static void msgbox_no_game_cb(lv_obj_t * msgbox, lv_event_t e);
static void game_list_cb(lv_obj_t * parent, lv_event_t e);

// On game menu
static void on_game_menu();
static void slider_volume_cb(lv_obj_t * slider, lv_event_t e);
static void slider_brightness_cb(lv_obj_t * slider, lv_event_t e);
static void list_game_menu_cb(lv_obj_t * parent, lv_event_t e);

// Wifi Library Manager menu
static void library_manager_menu(lv_obj_t * parent);
static void library_manager_cb(lv_obj_t * parent, lv_event_t e);

// Bluetooth Controller menu
void bt_controller_menu(lv_obj_t * parent);
static void bluetooth_controller_cb(lv_obj_t * parent, lv_event_t e);

// Configuration menu
void config_menu(lv_obj_t * parent);


/**********************
 *  GLOBAL VARIABLES
 **********************/
uint8_t tab_num = 0;

uint32_t btn_left_time = 0;
uint32_t btn_right_time = 0;
uint32_t btn_up_time = 0;
uint32_t btn_down_time = 0;
uint32_t btn_a_time = 0;
uint32_t btn_b_time = 0;
uint32_t btn_menu_time = 0;

uint8_t emulator_selected = 0x00;

bool sub_menu = false;

/**********************
 *  LVGL groups and objects
 **********************/

static lv_group_t * group_interact; // Group of interactive objects

static lv_indev_t * kb_indev;

// Notification bar container objects
static lv_obj_t * notification_cont;
static lv_obj_t * battery_bar;
static lv_obj_t * battery_label;
static lv_obj_t * WIFI_label;
static lv_obj_t * BT_label;
static lv_obj_t * SD_label;

// Main menu objects

static lv_obj_t * tab_main_menu;
static lv_obj_t * tab_emulators;
static lv_obj_t * tab_lib_manager;
static lv_obj_t * tab_bt_controller;
static lv_obj_t * tab_config;

// Emulators menu objects

static lv_obj_t * list_emulators_main;
static lv_obj_t * btn_emulator_lib;
static lv_obj_t * container_header_game_icon;

// On-game menu objects

static lv_obj_t * list_on_game;
static lv_obj_t * mbox_volume;
static lv_obj_t * mbox_brightness;

// Wifi-Library manager objects

static lv_obj_t * btn_wifi_manager;

// Bluetooth controller objects

static lv_obj_t * bt_controller_btn;

// Configuration menu objects

static lv_obj_t * config_btn;


static const char *TAG = "GUI_frontend";

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void GUI_frontend(void){

    group_interact = lv_group_create();

    //  Initalize keypad task to control user input.
    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = user_input_task;
    kb_indev = lv_indev_drv_register(&kb_drv);
    lv_indev_set_group(kb_indev, group_interact);

    /* Device State Bar */
    //This bar shows the battery status, if the SD card is attached
    // or if any wireless communication is active.
    notification_cont = lv_cont_create(lv_layer_top(), NULL);
    lv_obj_align_origo(notification_cont, NULL, LV_ALIGN_IN_TOP_LEFT, 35, 20);
    lv_obj_set_size(notification_cont,250,30);
    lv_cont_set_layout(notification_cont, LV_LAYOUT_OFF);

    /* Battery Bar */
    battery_bar = lv_bar_create(notification_cont, NULL);
    lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x0CC62D));
    lv_obj_set_style_local_border_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_obj_set_size(battery_bar, 40, 17);
    lv_obj_align_origo(battery_bar, NULL, LV_ALIGN_CENTER, 100, 0);
    lv_bar_set_value(battery_bar, 80, NULL);

    battery_label = lv_label_create(battery_bar, NULL);
    lv_obj_align_origo(battery_label, NULL, LV_ALIGN_CENTER, 7, 2);
    lv_obj_set_style_local_text_font(battery_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_12 );

    // This task checks every minute if a new battery message is on the queue
    lv_task_t * task = lv_task_create(battery_status_task, 1000, LV_TASK_PRIO_LOW, NULL);

    /* SD card connected icon */
    SD_label = lv_label_create(notification_cont, NULL);
    lv_label_set_text(SD_label, LV_SYMBOL_SD_CARD);
    lv_obj_align_origo(SD_label, NULL, LV_ALIGN_IN_LEFT_MID, 25, 0);
    // Check if is connected
    /* if(! sd_connected) lv_obj_set_hidden(SD_label,true); */

    //Wi-Fi Status Icon
    WIFI_label = lv_label_create(notification_cont, NULL);
    lv_label_set_text(WIFI_label, LV_SYMBOL_WIFI);
    lv_obj_align_origo(WIFI_label, NULL, LV_ALIGN_IN_LEFT_MID, 50, 0);
    lv_obj_set_hidden(WIFI_label,true);

    //Bluetooth Status Icon
    BT_label = lv_label_create(notification_cont, NULL);
    lv_label_set_text(BT_label, LV_SYMBOL_BLUETOOTH);
    lv_obj_align_origo(BT_label, NULL, LV_ALIGN_IN_LEFT_MID, 75, 0);
    lv_obj_set_hidden(BT_label,true);

    /* Tab menu shows the different sub menu:
        - Emulator library.
        - Wi-Fi Library Manager.
        - Bluetooth controller.
        - Configuration
    */

    tab_main_menu = lv_tabview_create(lv_scr_act(),NULL);
    lv_tabview_set_btns_pos(tab_main_menu,LV_TABVIEW_TAB_POS_NONE);

    tab_emulators = lv_tabview_add_tab(tab_main_menu,"Emulators");
    tab_lib_manager = lv_tabview_add_tab(tab_main_menu,"Wifi-Library-Manager");
    tab_bt_controller = lv_tabview_add_tab(tab_main_menu,"Bluetooth controller");
    tab_config = lv_tabview_add_tab(tab_main_menu,"Configuration");

    emulators_menu(tab_emulators);
    library_manager_menu(tab_lib_manager);
    bt_controller_menu(tab_bt_controller);
    config_menu(tab_config);


}


/**********************
 *   STATIC FUNCTIONS
 **********************/

/* Emulator menu function */

static void emulators_menu(lv_obj_t * parent){
    // Creation of the button with image on the main menu.

    btn_emulator_lib = lv_btn_create(parent, NULL);
    lv_obj_align(btn_emulator_lib, NULL, LV_ALIGN_CENTER, -15, -40);
    lv_obj_set_size(btn_emulator_lib,150,150);

    // Set the image and the sub label
    lv_obj_t * console_image = lv_img_create(btn_emulator_lib, NULL);
    lv_img_set_src(console_image, &emulators_icon);
    lv_obj_t * label = lv_label_create(btn_emulator_lib, NULL);
    lv_label_set_text(label, "Emulators");

    lv_obj_set_event_cb(btn_emulator_lib, emulators_list_cb);

    // Add to the available objects group to manipulate it. 
    lv_group_add_obj(group_interact, btn_emulator_lib);
}

/* On game menu function */

static void on_game_menu(){
    sub_menu = true;

    lv_obj_t * list_on_game = lv_list_create(lv_layer_top(), NULL);
    lv_obj_set_size(list_on_game, 240, 240);
    lv_obj_align(list_on_game, NULL, LV_ALIGN_CENTER, 0, 0);

    
    lv_obj_t * list_btn;

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_HOME, "Resume Game");
    lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_SAVE, "Save Game");
    lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_VOLUME_MAX, "Volume");
    lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_IMAGE, "Brightness");
    lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_on_game, LV_SYMBOL_CLOSE, "Exit");
    lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    lv_group_add_obj(group_interact, list_on_game);
    lv_group_focus_obj(list_on_game);

}


/* Wi-Fi library manager function */

static void library_manager_menu(lv_obj_t * parent){

    btn_wifi_manager = lv_btn_create(parent, NULL);
    lv_obj_align(btn_wifi_manager, NULL, LV_ALIGN_CENTER, -15, -40);
    lv_obj_set_size(btn_wifi_manager,150,150);

    lv_obj_t * wifi_lib_image = lv_img_create(btn_wifi_manager, NULL);
    lv_img_set_src(wifi_lib_image, &wifi_lib_icon);

    lv_obj_t * label = lv_label_create(btn_wifi_manager, NULL);
    lv_label_set_text(label, "Wi-Fi Library Manager");
    lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label, 100);
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 25, 0);

    lv_obj_set_event_cb(btn_wifi_manager, library_manager_cb); 
    lv_group_add_obj(group_interact, btn_wifi_manager);

}

/* Bluetooth controller main function */

void bt_controller_menu(lv_obj_t * parent){

    bt_controller_btn = lv_btn_create(parent, NULL);

    lv_obj_align(bt_controller_btn, NULL, LV_ALIGN_CENTER, -15, -40);
    lv_obj_set_size(bt_controller_btn,150,150);

    lv_obj_t * bt_cotroller_image = lv_img_create(bt_controller_btn, NULL);
    lv_img_set_src(bt_cotroller_image, &bt_controller_icon);

    lv_obj_t * label = lv_label_create(bt_controller_btn, NULL);
    lv_label_set_text(label, "Bluetooth controller");
    lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label, 100);
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 25, 0);

    lv_obj_set_event_cb(bt_controller_btn, bluetooth_controller_cb); 
    lv_group_add_obj(group_interact, bt_controller_btn);
}

/* System Configuration main function */

void config_menu(lv_obj_t * parent){
    config_btn = lv_btn_create(parent, NULL);
    lv_obj_align(config_btn, NULL, LV_ALIGN_CENTER, -15, -40);
    lv_obj_set_size(config_btn,150,150);
    lv_obj_t * wifi_lib_image = lv_img_create(config_btn, NULL);
    lv_img_set_src(wifi_lib_image, &settings_icon);
    lv_obj_t * label = lv_label_create(config_btn, NULL);
    lv_label_set_text(label, "Configuration");

   // lv_obj_set_event_cb(config_btn, game_menu_event_cb); 
    lv_group_add_obj(group_interact, config_btn);
}


/**********************
 *  CALL-BACK FUNCTIONS
 **********************/

/* Emulator menu call-back functions */

static void game_list_cb(lv_obj_t * parent, lv_event_t e){

    // Get the list of available games for each console and show a header with the icon of the console
    if(e == LV_EVENT_CLICKED){
        
        container_header_game_icon = lv_cont_create(lv_layer_top(), NULL);
        lv_obj_align(container_header_game_icon, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);
        lv_obj_set_size(container_header_game_icon,240,40);
        lv_cont_set_layout(container_header_game_icon, LV_LAYOUT_OFF);

        lv_obj_t * console_label = lv_label_create(container_header_game_icon, NULL);
        lv_obj_t * console_image = lv_img_create(container_header_game_icon, NULL);

        lv_obj_set_style_local_text_font(console_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_20 );


        // Check which console was selected and create the title
        if(strcmp(lv_list_get_btn_text(parent),"NES")==0){
            emulator_selected = NES;

            lv_label_set_text(console_label, "NES");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &nes_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -40, 0);

            lv_obj_set_hidden(list_emulators_main,true);
            ESP_LOGI(TAG,"Selected NES");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"SNES")==0){
            emulator_selected = SNES;

            lv_label_set_text(console_label, "SNES");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &snes_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -40, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected SNES");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"GameBoy")==0){
            emulator_selected = GAMEBOY;

            lv_label_set_text(console_label, "GameBoy");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &gameboy_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -40, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected GameBoy");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"GameBoy Color")==0){
            emulator_selected = GAMEBOY_COLOR;

            lv_label_set_text(console_label, "GameBoy Color");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &gameboycolor_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -40, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected GameBoy Color");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Master System")==0){
            emulator_selected = SMS;

            lv_label_set_text(console_label, "Master System");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &sega_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -40, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected Sega MasterSystem");
        }

        // Get the game list of each console.
        char game_name[30][100];
        uint8_t games_num = sd_game_list(game_name, emulator_selected);

        ESP_LOGI(TAG,"Found %i games",games_num);

        // Print the list of games or show a message is any game is available
        if(games_num>0){
            lv_obj_t *  list_game_emulator = lv_list_create(lv_layer_top(), NULL);
            lv_obj_set_size(list_game_emulator, 210, 200);
            lv_obj_align(list_game_emulator, NULL, LV_ALIGN_CENTER, 0, 23);
            lv_obj_set_event_cb(list_game_emulator, emulator_list_event);
            lv_page_glue_obj(list_game_emulator,true);
            // Add a button for each game
            for(int i=0;i<games_num;i++){
                lv_obj_t * game_btn = lv_list_add_btn(list_game_emulator, NULL, game_name[i]);
                lv_group_add_obj(group_interact, game_btn);
                lv_obj_set_event_cb(game_btn, game_execute_cb);
            }
            lv_group_add_obj(group_interact, list_game_emulator);

            lv_group_focus_obj(list_game_emulator);
        }
        else{
            lv_obj_del(container_header_game_icon);

            lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox, "Oops! Any game available.");
            lv_obj_set_event_cb(mbox, msgbox_no_game_cb);
            lv_group_add_obj(group_interact, mbox);
            lv_group_focus_obj(mbox);
            lv_group_focus_freeze(group_interact, true);

            lv_obj_t * nogame_image = lv_img_create(mbox, NULL);
            lv_img_set_src(nogame_image, &nogame_icon);
            lv_obj_align(nogame_image, mbox, LV_ALIGN_CENTER, 0, 0);

            static const char * btns[] = {"Ok", "", ""};
            lv_msgbox_add_btns(mbox, btns);
            lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);
        }
        
    }
}

static void msgbox_no_game_cb(lv_obj_t * msgbox, lv_event_t e){
    // Delete the message of no games
    if(e == LV_EVENT_CLICKED) {
        lv_obj_del(msgbox);
        lv_obj_reset_style_list(lv_layer_top(), LV_OBJ_PART_MAIN);
        lv_obj_set_click(lv_layer_top(), false);
    }
}

static void emulators_list_cb(lv_obj_t * parent, lv_event_t e){
    // Create a list with the available emulators
    if(e == LV_EVENT_CLICKED){

        list_emulators_main = lv_list_create(lv_layer_top(), NULL);
        lv_obj_set_size(list_emulators_main, 210, 200);
        lv_obj_align(list_emulators_main, NULL, LV_ALIGN_CENTER, 0, 10);
        lv_obj_set_event_cb(list_emulators_main, emulator_list_event);

        lv_obj_t * emulator_NES_btn = lv_list_add_btn(list_emulators_main,  &nes_icon, "NES");
        lv_obj_set_size(emulator_NES_btn, 200, 10);
        lv_obj_set_event_cb(emulator_NES_btn, game_list_cb);

        lv_obj_t * emulator_SNES_btn = lv_list_add_btn(list_emulators_main,  &snes_icon, "SNES");
        lv_obj_set_size(emulator_SNES_btn, 200, 10);
        lv_obj_set_event_cb(emulator_SNES_btn, game_list_cb);

        lv_obj_t * emulator_GB_btn = lv_list_add_btn(list_emulators_main,  &gameboy_icon, "GameBoy");
        lv_obj_set_size(emulator_GB_btn, 200, 20);
        lv_obj_set_event_cb(emulator_GB_btn, game_list_cb);

        lv_obj_t * emulator_GBC_btn = lv_list_add_btn(list_emulators_main,  &gameboycolor_icon, "GameBoy Color");
        lv_obj_set_size(emulator_GBC_btn, 200, 20);
        lv_obj_set_event_cb(emulator_GBC_btn, game_list_cb);

        lv_obj_t * emulator_SMS_btn = lv_list_add_btn(list_emulators_main,  &sega_icon, "Master System");
        lv_obj_set_size(emulator_SMS_btn, 200, 10);
        lv_obj_set_event_cb(emulator_SMS_btn, game_list_cb);
        
        lv_group_add_obj(group_interact, list_emulators_main);
        lv_group_focus_obj(list_emulators_main);



    }
}

static void emulator_list_event(lv_obj_t * parent, lv_event_t e){
    // Delete the list of emulators
    if(e == LV_EVENT_CANCEL  ){
        lv_obj_del(parent);
    }
}

static void game_execute_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Loading: %s",(char *)lv_list_get_btn_text(parent));

        struct SYSTEM_MODE emulator;
        emulator.mode = MODE_GAME;
        emulator.status = 1;
        emulator.console = emulator_selected;
        strcpy(emulator.game_name, (char *)lv_list_get_btn_text(parent));

        if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG, "Error sending game execution queue");
        }

        on_game_menu();

    }
    else if(e == LV_EVENT_CANCEL  ){
        // Delete the list of games and the header icon
        lv_obj_del(container_header_game_icon);
        lv_obj_set_hidden(list_emulators_main,false);
    }
}


/* On game menu call-back functions */


static void slider_volume_cb(lv_obj_t * slider, lv_event_t e){
    if(e == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "Volumen set: %i",lv_slider_get_value(slider));
        audio_volume_set(lv_slider_get_value(slider));
    }
    else if(e == LV_EVENT_CANCEL){
        lv_obj_del(mbox_volume);
    }
}

static void slider_brightness_cb(lv_obj_t * slider, lv_event_t e){
    if(e == LV_EVENT_VALUE_CHANGED) {
        ESP_LOGI(TAG, "Brightness set: %i",lv_slider_get_value(slider));
        st7789_backlight_set(lv_slider_get_value(slider));
    }
    else if(e == LV_EVENT_CANCEL){
        lv_obj_del(mbox_brightness);
    }
}

static void list_game_menu_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){
        struct SYSTEM_MODE emulator;

        if(strcmp(lv_list_get_btn_text(parent),"Resume Game")==0){
            
            emulator.mode = MODE_GAME;
            emulator.status = 0;

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"modeQueue send error");
            }

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Save Game")==0){

            emulator.mode = MODE_SAVE_GAME;

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"modeQueue send error");
            }

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Volume")==0){
            
            mbox_volume = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_volume, "Volume");
            
            lv_group_add_obj(group_interact, mbox_volume);
            lv_group_focus_obj(mbox_volume);

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);
            

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);

            lv_obj_t * slider = lv_slider_create(mbox_volume, NULL);
            lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            // Get volumen
            uint8_t volume = audio_volume_get();
            lv_slider_set_value(slider,volume,LV_ANIM_OFF);

            lv_obj_set_event_cb(slider, slider_volume_cb);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Brightness")==0){
            mbox_brightness = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_brightness, "Brightness");
            
            lv_group_add_obj(group_interact, mbox_brightness);
            lv_group_focus_obj(mbox_brightness);

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);
            

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);

            lv_obj_t * slider = lv_slider_create(mbox_brightness, NULL);
            lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            
            uint8_t brightness_level = st7789_backlight_get();
            lv_slider_set_value(slider,brightness_level,LV_ANIM_ON);

            lv_obj_set_event_cb(slider, slider_brightness_cb);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Exit")==0){

            emulator.mode = MODE_OUT;

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"modeQueue send error");
            }

        }
    }
    else if(e == LV_EVENT_CANCEL){
        // Pushed button B, so whe want to close the menu
        struct SYSTEM_MODE emulator;
        emulator.mode = MODE_GAME;
        emulator.status = 0;

        if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG,"SYSTE Mode queue send error");
        }
    }
}


/* Wifi library manager call-back functions */

static void library_manager_cb(lv_obj_t * parent, lv_event_t e){

    // TODO: Add menu information
    if(e == LV_EVENT_CLICKED){
        struct SYSTEM_MODE emulator;
        emulator.mode = MODE_WIFI_LIB;
        emulator.status = 1;
        emulator.console = NULL;

        if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG," Wifi Lib manager queue send fail");
        }
    }
}

/* Bluetooth ctonrller call-back functions */

static void bluetooth_controller_cb(lv_obj_t * parent, lv_event_t e){

    if(e == LV_EVENT_CLICKED){
        struct SYSTEM_MODE emulator;
        emulator.mode = MODE_BT_CONTROLLER;
        emulator.status = 1;
        emulator.console = NULL;

        if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG," Bluetooth controller queue send fail");
        }
    }


}


/**********************
 *   TASKS FUNCTIONS
 **********************/

static void battery_status_task(lv_task_t * task){
    // This task check every minute if the battery level has change. If it receive a message from the 
    // queue, it change the text value and the size of the battery bar.

    struct BATTERY_STATUS management;

    if(xQueueReceive(batteryQueue, &management,( TickType_t ) 0)){

        char battery_level[4];
        sprintf(battery_level,"%i",management.percentage);

        lv_label_set_text(battery_label, battery_level);
        lv_bar_set_value(battery_bar, management.percentage, NULL);

        // If the battery is equal or less than the 25% the bar change to red, otherwise the bar is green.
        if(management.percentage <= 25){
            lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0xC61D0C));
        }
        else{
            lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x0CC62D));
        }
    }
}



static bool user_input_task(lv_indev_drv_t * indev_drv, lv_indev_data_t * data){

    // Get the status of the multiplexer driver
    uint16_t inputs_value =  input_read();

    if(!((inputs_value >> 0) & 0x01)){
        printf("Down\r\n");
        // Button Down pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        // To avoid bounce with the buttons, we need to wait 2 ms.
        if((actual_time-btn_down_time)>2){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_DOWN;

            // Save the actual time to calculate the bounce time.
            btn_down_time = actual_time;
        }
    }

    if(!((inputs_value >> 1) & 0x01)){
        // Button Left pushed
        printf("left\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;
        if((actual_time-btn_left_time)>5){

            if(sub_menu){
                data->state = LV_INDEV_STATE_PR;
                data->key = LV_KEY_LEFT;
                btn_left_time = actual_time;
            }
            else{
                tab_num--;
                if(tab_num <= 0) tab_num = 0;
                else{
                    printf("prev %i\r\n",tab_num);
                    data->state = LV_INDEV_STATE_PR;
                    data->key = LV_KEY_PREV;
                    lv_tabview_set_tab_act(tab_main_menu, tab_num, LV_ANIM_ON);
                }
                btn_left_time = actual_time;
            }
        }
    }

    if(!((inputs_value >> 2) & 0x01)){
        // Button left pushed
        printf("up\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;
        if((actual_time-btn_up_time)>2){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_UP;
            btn_up_time = actual_time;
        }
         
    }

    if(!((inputs_value >> 3) & 0x01)){
        // Button right pushed
         printf("right\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_right_time)>5){

            if(sub_menu){
                data->state = LV_INDEV_STATE_PR;
                data->key = LV_KEY_RIGHT;
                btn_right_time = actual_time;
            }
            else{
                tab_num++;
                if(tab_num >= 4) tab_num = 4;
                else{
                    data->state = LV_INDEV_STATE_PR;
                    data->key = LV_KEY_NEXT;
                    lv_tabview_set_tab_act(tab_main_menu, tab_num, LV_ANIM_ON);
                }
                btn_right_time = actual_time;
            }
        }    
    }

    if(!((inputs_value >> 11) & 0x01)){
        // Button menu pushed
        printf("Menu\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_menu_time)>5){
            struct SYSTEM_MODE emulator;
            emulator.mode = MODE_GAME;
            emulator.status = 0;

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"Button menu queue send error");
            }

            btn_menu_time = actual_time;
        }
    }

    if(!((inputs_value >> 8) & 0x01)){
        // Button B pushed
        printf("B\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_b_time)>5){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_ESC;
            btn_b_time = actual_time;
        }
    }

    if(!((inputs_value >> 9) & 0x01)){
        // Button A pushed
        printf("A\r\n");
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_a_time)>5){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_ENTER;
            btn_a_time = actual_time;
        }
    }

    return false;
}
