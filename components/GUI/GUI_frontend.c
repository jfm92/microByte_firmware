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
#include "backlight_ctrl.h"
#include "battery.h"

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
#include "icons/GG_icon.h"

/* Wi-Fi lib manager icons */
#include "icons/ext_application_icon.h"

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
static void game_menu_cb(lv_obj_t * parent, lv_event_t e);
static void msgbox_no_game_cb(lv_obj_t * msgbox, lv_event_t e);
static void game_list_cb(lv_obj_t * parent, lv_event_t e);

// On game menu
static void on_game_menu();
static void slider_volume_cb(lv_obj_t * slider, lv_event_t e);
static void slider_brightness_cb(lv_obj_t * slider, lv_event_t e);
static void list_game_menu_cb(lv_obj_t * parent, lv_event_t e);

// External app menu
static void external_app_menu(lv_obj_t * parent);
static void external_app_cb(lv_obj_t * parent, lv_event_t e);
static void app_execute_cb(lv_obj_t * parent, lv_event_t e);

// Configuration menu
void config_menu(lv_obj_t * parent);
static void configuration_cb(lv_obj_t * parent, lv_event_t e);
static void config_option_cb(lv_obj_t * parent, lv_event_t e);
static void mbox_config_cb(lv_obj_t * parent, lv_event_t e);
static void fw_update_cb(lv_obj_t * parent, lv_event_t e);

//Extra functions
static void GUI_theme_color(uint8_t color_selected);

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

#define bouncing_time 7 //MS to wait to avoid bouncing on button selection

uint8_t emulator_selected = 0x00;

bool sub_menu = false;

// TODO: Get this configuration from the ROM
uint32_t GUI_THEME = LV_THEME_MATERIAL_FLAG_LIGHT;

struct BATTERY_STATUS management;


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
static lv_obj_t * Charging_label;

// Main menu objects

static lv_obj_t * tab_main_menu;
static lv_obj_t * tab_emulators;
static lv_obj_t * tab_ext_app_manager;
static lv_obj_t * tab_bt_controller;
static lv_obj_t * tab_config;

// Emulators menu objects

static lv_obj_t * list_emulators_main;
static lv_obj_t * btn_emulator_lib;
static lv_obj_t * container_header_game_icon;
static lv_obj_t * list_game_emulator;
static lv_obj_t * list_game_options;
static lv_obj_t * mbox_game_options;
//Buttons of the game initialization list
static lv_obj_t * btn_game_new;
static lv_obj_t * btn_game_resume;
static lv_obj_t * btn_game_delete;

// On-game menu objects

static lv_obj_t * list_on_game;
static lv_obj_t * mbox_volume;
static lv_obj_t * mbox_brightness;

// External app menu objects

static lv_obj_t * btn_ext_app;
static lv_obj_t * list_external_app;


// Configuration menu objects

static lv_obj_t * config_btn;
static lv_obj_t * list_config;
static lv_obj_t * list_fw_update;
static lv_obj_t * mbox_about;
static lv_obj_t * mbox_color;


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
    

    battery_label = lv_label_create(battery_bar, NULL);
    lv_obj_align_origo(battery_label, NULL, LV_ALIGN_CENTER, 7, 2);
    lv_obj_set_style_local_text_font(battery_label, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_12 );
    
    //To show the battery level before the first message arrives to the queue
    uint8_t battery_aux = battery_get_percentage();
    char *battery_level = malloc(4); 
    sprintf(battery_level,"%i",battery_aux);
    lv_label_set_text(battery_label, battery_level);
    lv_bar_set_value(battery_bar, battery_aux, NULL);
    free(battery_level);

    // This task checks every minute if a new battery message is on the queue
    lv_task_t * task = lv_task_create(battery_status_task, 1000, LV_TASK_PRIO_LOW, NULL);

    /* SD card connected icon */
    SD_label = lv_label_create(notification_cont, NULL);
    lv_label_set_text(SD_label, LV_SYMBOL_SD_CARD);
    lv_obj_align_origo(SD_label, NULL, LV_ALIGN_IN_LEFT_MID, 25, 0);
    // Check if is connected
    if(!sd_mounted()) lv_obj_set_hidden(SD_label,true); 

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

    //Bluetooth Status Icon
    Charging_label = lv_label_create(notification_cont, NULL);
    lv_label_set_text(Charging_label, LV_SYMBOL_CHARGE);
    lv_obj_align_origo(Charging_label, NULL, LV_ALIGN_IN_LEFT_MID, 190, 0);
    lv_obj_set_hidden(Charging_label,true);


    /* Tab menu shows the different sub menu:
        - Emulator library.
        - Wi-Fi Library Manager.
        - Bluetooth controller.
        - Configuration
    */

    tab_main_menu = lv_tabview_create(lv_scr_act(),NULL);
    lv_tabview_set_btns_pos(tab_main_menu,LV_TABVIEW_TAB_POS_NONE);

    tab_emulators = lv_tabview_add_tab(tab_main_menu,"Emulators");
    tab_ext_app_manager = lv_tabview_add_tab(tab_main_menu,"External Application");
    tab_config = lv_tabview_add_tab(tab_main_menu,"Configuration");

    //Set the saved color configuration
    uint8_t theme_color = system_get_config(SYS_GUI_COLOR);
    GUI_theme_color(theme_color);

    emulators_menu(tab_emulators);
    external_app_menu(tab_ext_app_manager);
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

static void emulators_game_menu(char * game_name){
 /*   lv_obj_t * list_game_menu = lv_list_create(lv_layer_top(), NULL);
    lv_obj_set_size(list_list_game_menuon_game, 180, 240);
    lv_obj_align(list_game_menu, NULL, LV_ALIGN_CENTER, 0, 0);

     lv_obj_t * list_btn;

    list_btn = lv_list_add_btn(list_game_menu, LV_SYMBOL_HOME, "New Game");
   // lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_game_menu, LV_SYMBOL_SAVE, "Load Game");
    //lv_obj_set_event_cb(list_btn, list_game_menu_cb);

    list_btn = lv_list_add_btn(list_game_menu, LV_SYMBOL_CLOSE, "Exit");
   // lv_obj_set_event_cb(list_btn, list_game_menu_cb);*/
}

/* On game menu function */

static void on_game_menu(){
    sub_menu = true;

    list_on_game = lv_list_create(lv_layer_top(), NULL);
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

static void external_app_menu(lv_obj_t * parent){

    btn_ext_app = lv_btn_create(parent, NULL);
    lv_obj_align(btn_ext_app, NULL, LV_ALIGN_CENTER, -15, -40);
    lv_obj_set_size(btn_ext_app,150,150);

    lv_obj_t * wifi_lib_image = lv_img_create(btn_ext_app, NULL);
    lv_img_set_src(wifi_lib_image, &ext_application_icon);

    lv_obj_t * label = lv_label_create(btn_ext_app, NULL);
    lv_label_set_text(label, "External Aplications");
    lv_label_set_long_mode(label, LV_LABEL_LONG_SROLL_CIRC);
    lv_obj_set_width(label, 100);
    lv_obj_align(label, NULL, LV_ALIGN_CENTER, 25, 0);

    lv_obj_set_event_cb(btn_ext_app, external_app_cb); 
    lv_group_add_obj(group_interact, btn_ext_app);

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

    lv_obj_set_event_cb(config_btn, configuration_cb); 
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
            lv_img_set_src(console_image, &NES_icon);
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
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 20, 0);
            lv_img_set_src(console_image, &gameboycolor_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -75, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected GameBoy Color");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Master System")==0){
            emulator_selected = SMS;

            lv_label_set_text(console_label, "Master System");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &sega_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -75, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected Sega MasterSystem");
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Game Gear")==0){
            emulator_selected = GG;

            lv_label_set_text(console_label, "Game Gear");
            lv_obj_align(console_label, container_header_game_icon, LV_ALIGN_CENTER, 25, 0);
            lv_img_set_src(console_image, &GG_icon);
            lv_obj_align(console_image, container_header_game_icon, LV_ALIGN_CENTER, -75, 0);

            lv_obj_set_hidden(list_emulators_main,true);

            ESP_LOGI(TAG,"Selected Sega Game Gear");
        }

        // Get the game list of each console.
        char *game_list[100];
        uint8_t games_num = sd_game_list(game_list, emulator_selected);

        ESP_LOGI(TAG,"Found %i games",games_num);

        // Print the list of games or show a message is any game is available
        if(games_num>0){
            list_game_emulator = lv_list_create(lv_layer_top(), NULL);
            lv_obj_set_size(list_game_emulator, 210, 200);
            lv_obj_align(list_game_emulator, NULL, LV_ALIGN_CENTER, 0, 23);
            //lv_obj_set_event_cb(list_game_emulator, emulator_list_event);
            lv_page_glue_obj(list_game_emulator,true);
            // Add a button for each game
            for(int i=0;i<games_num;i++){
                lv_obj_t * game_btn = lv_list_add_btn(list_game_emulator, NULL, game_list[i]);
                lv_group_add_obj(group_interact, game_btn);
                lv_obj_set_event_cb(game_btn, game_menu_cb);
                free(game_list[i]);
            }
            lv_group_add_obj(group_interact, list_game_emulator);

            lv_group_focus_obj(list_game_emulator);
        }
        else{
            lv_obj_del(container_header_game_icon);

            lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox, "Oops! No games available.");
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
    else if(e == LV_EVENT_CANCEL ){
        sub_menu = false;
        lv_group_focus_obj(btn_emulator_lib);
        lv_obj_del(list_emulators_main);
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
        sub_menu = true;
        list_emulators_main = lv_list_create(lv_layer_top(), NULL);
        lv_obj_set_size(list_emulators_main, 210, 200);
        lv_obj_align(list_emulators_main, NULL, LV_ALIGN_CENTER, 0, 10);
        //lv_obj_set_event_cb(list_emulators_main, emulator_list_event);

        lv_obj_t * emulator_NES_btn = lv_list_add_btn(list_emulators_main,  &NES_icon, "NES");
        lv_obj_set_size(emulator_NES_btn, 200, 10);
        lv_obj_set_event_cb(emulator_NES_btn, game_list_cb);

        lv_obj_t * emulator_GB_btn = lv_list_add_btn(list_emulators_main,  &gameboy_icon, "GameBoy");
        lv_obj_set_size(emulator_GB_btn, 200, 20);
        lv_obj_set_event_cb(emulator_GB_btn, game_list_cb);

        lv_obj_t * emulator_GBC_btn = lv_list_add_btn(list_emulators_main,  &gameboycolor_icon, "GameBoy Color");
        lv_obj_set_size(emulator_GBC_btn, 200, 20);
        lv_obj_set_event_cb(emulator_GBC_btn, game_list_cb);

        lv_obj_t * emulator_SMS_btn = lv_list_add_btn(list_emulators_main,  &sega_icon, "Master System");
        lv_obj_set_size(emulator_SMS_btn, 200, 10);
        lv_obj_set_event_cb(emulator_SMS_btn, game_list_cb);

        lv_obj_t * emulator_GG_btn = lv_list_add_btn(list_emulators_main,  &GG_icon, "Game Gear");
        lv_obj_set_size(emulator_GG_btn, 200, 10);
        lv_obj_set_event_cb(emulator_GG_btn, game_list_cb);
        
        lv_group_add_obj(group_interact, list_emulators_main);
        lv_group_focus_obj(list_emulators_main);
    }
}

static void game_menu_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED) {
       
        lv_obj_set_hidden(container_header_game_icon,true);
        lv_obj_set_hidden(list_emulators_main,true);
        lv_obj_set_hidden(list_game_emulator,true);
        
        mbox_game_options = lv_msgbox_create(lv_layer_top(), NULL);
        lv_msgbox_set_text(mbox_game_options, (char *)lv_list_get_btn_text(parent));
        lv_obj_align(mbox_game_options, NULL, LV_ALIGN_CENTER, 0, -70);
        

        lv_obj_set_click(lv_layer_top(), true);

        list_game_options = lv_list_create(mbox_game_options, NULL);

        btn_game_new = lv_list_add_btn(list_game_options, LV_SYMBOL_HOME, "New Game");
        lv_obj_set_event_cb(btn_game_new, game_execute_cb);

        if(sd_sav_exist((char *)lv_list_get_btn_text(parent),emulator_selected)){
            btn_game_resume = lv_list_add_btn(list_game_options, LV_SYMBOL_SAVE, "Resume Game");
            lv_obj_set_event_cb(btn_game_resume, game_execute_cb);

            btn_game_delete = lv_list_add_btn(list_game_options, LV_SYMBOL_CLOSE, "Delete Save Data");
            lv_obj_set_event_cb(btn_game_delete, game_execute_cb);
        }
        

        lv_group_add_obj(group_interact, list_game_options);
        lv_group_focus_obj(list_game_options);

        //TODO: Get player time



    }
    else if(e == LV_EVENT_CANCEL  ){
        // Delete the list of games and the header icon
        lv_obj_del(container_header_game_icon);
        lv_obj_del(list_game_emulator);
        lv_obj_set_hidden(list_emulators_main,false);
        lv_group_focus_obj(list_emulators_main);
    }
}

static void game_execute_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED) {
        struct SYSTEM_MODE emulator;
        if(strcmp(lv_list_get_btn_text(parent),"New Game")==0){
            ESP_LOGI(TAG, "Loading: %s",(char *)lv_msgbox_get_text(mbox_game_options));
 
            
            emulator.mode = MODE_GAME;
            emulator.load_save_game = 0;
            emulator.status = 1;
            emulator.console = emulator_selected;
            strcpy(emulator.game_name, (char *)lv_msgbox_get_text(mbox_game_options));

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG, "Error sending game execution queue");
            }

            on_game_menu();

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Resume Game")==0){
            ESP_LOGI(TAG, "Loading: %s",(char *)lv_msgbox_get_text(mbox_game_options));
 
            emulator.mode = MODE_GAME;
            emulator.load_save_game = 1;
            emulator.status = 1;
            emulator.console = emulator_selected;
            strcpy(emulator.game_name, (char *)lv_msgbox_get_text(mbox_game_options));

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG, "Error sending game execution queue");
            }
            on_game_menu();

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Delete Save Data")==0){
            sd_sav_remove((char *)lv_msgbox_get_text(mbox_game_options), emulator_selected);
            //Update the list of options
            lv_obj_del(btn_game_delete);
            lv_obj_del(btn_game_resume);
        }

        //Get the name of the game from the text of the message box
        

    }
    else if(e == LV_EVENT_CANCEL) {
        lv_obj_set_hidden(container_header_game_icon,false);
        lv_obj_set_hidden(list_emulators_main,false);
        lv_obj_set_hidden(list_game_emulator,false);

        lv_obj_del(mbox_game_options);
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
        backlight_set(lv_slider_get_value(slider));
    }
    else if(e == LV_EVENT_CANCEL){
        lv_obj_del(mbox_brightness);
    }
}

static void list_game_menu_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){
        struct SYSTEM_MODE emulator;

        if(strcmp(lv_list_get_btn_text(parent),"Resume Game")==0){
            printf("Resume game\r\n");
            emulator.mode = MODE_GAME;
            emulator.status = 0;

            if( xQueueSend( modeQueue,&emulator, ( TickType_t ) 10) != pdPASS ){
                ESP_LOGE(TAG,"modeQueue send error");
            }

        }
        else if(strcmp(lv_list_get_btn_text(parent),"Save Game")==0){

            emulator.mode = MODE_SAVE_GAME;
            emulator.console = emulator_selected;

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

            lv_obj_t * slider = lv_slider_create(mbox_brightness, NULL);
            lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            
            uint8_t brightness_level = backlight_get();
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


/* External app call-back functions */

static void external_app_cb(lv_obj_t * parent, lv_event_t e){

    if(e == LV_EVENT_CLICKED){

        char *app_list[100];
        uint8_t app_num = sd_app_list(app_list,false);

        ESP_LOGI(TAG,"Found %i applications",app_num);

        if(app_num > 0){
            //Create a list of applications
            sub_menu = true;
            list_external_app = lv_list_create(lv_layer_top(), NULL);
            lv_obj_set_size(list_external_app, 210, 200);
            lv_obj_align(list_external_app, NULL, LV_ALIGN_CENTER, 0, 10);

             lv_page_glue_obj(list_external_app,true);
            // Add a button for each game
            for(int i=0;i<app_num;i++){
                lv_obj_t * app_btn = lv_list_add_btn(list_external_app, NULL, app_list[i]);
                lv_group_add_obj(group_interact, app_btn);
                lv_obj_set_event_cb(app_btn, app_execute_cb);
                free(app_list[i]);
            }
            lv_group_add_obj(group_interact, list_external_app);

            lv_group_focus_obj(list_external_app);

        }
        else{
            //TODO: Repair where it focus this message once is close
            //Show a message
            lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox, "No apps available.");
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

            lv_group_focus_obj(btn_ext_app);
        }
    }
}

static void app_execute_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){
        
        ESP_LOGI(TAG, "Loading: %s",(char *)lv_list_get_btn_text(parent));
        
        lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
        lv_msgbox_set_text(mbox, "Loading:");
        lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, -60);

        lv_obj_t * label1 = lv_label_create(mbox, NULL);
        lv_label_set_text(label1,lv_list_get_btn_text(parent));

        lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
        lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_set_click(lv_layer_top(), true);

        lv_group_focus_obj(mbox);
        lv_group_focus_freeze(group_interact, false);

        lv_obj_t * preload = lv_spinner_create(mbox, NULL);
        lv_obj_set_size(preload, 100, 100);
        lv_obj_align(preload, NULL, LV_ALIGN_CENTER, 0, 0);

        struct SYSTEM_MODE ext_app;
        ext_app.mode = MODE_EXT_APP;
        strcpy(ext_app.game_name,lv_list_get_btn_text(parent));

        if( xQueueSend( modeQueue,&ext_app, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG," External app queue send fail");
        }

        lv_obj_del(list_external_app);
        
    }
    else if(e == LV_EVENT_CANCEL){
        sub_menu = false;
        lv_obj_del(list_external_app);
        lv_group_focus_obj(btn_ext_app);
    }
}

/* Configuration menu cb function */

static void configuration_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){

        sub_menu = true;

        list_config = lv_list_create(lv_layer_top(), NULL);
        lv_obj_set_size(list_config, 225, 200);
        lv_obj_align(list_config, NULL, LV_ALIGN_CENTER, 0, 15);

        
        lv_obj_t * list_btn;

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SETTINGS, "About this device");
        lv_obj_set_event_cb(list_btn, config_option_cb);

        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_DOWNLOAD, "Update firmware");
        lv_obj_set_event_cb(list_btn, config_option_cb);

        //System config options
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_EYE_OPEN, "Brightness");
        lv_obj_set_event_cb(list_btn, config_option_cb);
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_IMAGE, "GUI Color Mode");
        lv_obj_set_event_cb(list_btn, config_option_cb);
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_VOLUME_MAX, "Volume");
        lv_obj_set_event_cb(list_btn, config_option_cb);
        if(system_get_config(SYS_STATE_SAV_BTN)!=1) list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SAVE, "Enable Button State Save");
        else if(system_get_config(SYS_STATE_SAV_BTN)) list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SAVE, "Disable Button State Save");
        lv_obj_set_event_cb(list_btn, config_option_cb);

        //System info options
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_BATTERY_FULL, "Battery Status");
        lv_obj_set_event_cb(list_btn, config_option_cb);
        list_btn = lv_list_add_btn(list_config, LV_SYMBOL_SD_CARD, "SD card Status");
        lv_obj_set_event_cb(list_btn, config_option_cb);

        lv_group_add_obj(group_interact, list_config);
        lv_group_focus_obj(list_config);
    }
}

static void config_option_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){

        if(strcmp(lv_list_get_btn_text(parent),"About this device")==0){

            mbox_about = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_about, "microByte");

            lv_obj_align(mbox_about, NULL, LV_ALIGN_CENTER, 0, -50);

            char *aux_text;
            lv_obj_t * label_hw_info = lv_label_create(mbox_about, NULL);
            aux_text = malloc(256);
            sprintf(aux_text,"\u2022 CPU: %s\n\u2022 RAM: %i MB \n\u2022 Flash: %i MB",cpu_version,RAM_size,FLASH_size);
            lv_label_set_text(label_hw_info,aux_text);
            free(aux_text);

            lv_obj_t * label_fw_version = lv_label_create(mbox_about, NULL);
            aux_text = malloc(256); 
            sprintf(aux_text,"%s",app_version);
            lv_label_set_text(label_fw_version,aux_text);
            free(aux_text);

            lv_group_add_obj(group_interact, mbox_about);
            lv_obj_set_event_cb(mbox_about, mbox_config_cb);
            lv_group_focus_obj(mbox_about);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Update firmware")==0){

                // Get the game list of each console.
            char fw_update_name[30][100];
            uint8_t fw_update_num = sd_app_list(fw_update_name,true);

            ESP_LOGI(TAG,"Found %i updates",fw_update_num);

            // Print the list of games or show a message is any game is available
            if(fw_update_num>0){
                list_fw_update = lv_list_create(lv_layer_top(), NULL);
                lv_obj_set_size(list_fw_update, 210, 210);
                lv_obj_align(list_fw_update, NULL, LV_ALIGN_CENTER, 0, 23);
                //lv_obj_set_event_cb(list_game_emulator, emulator_list_event);
                lv_page_glue_obj(list_fw_update,true);
                // Add a button for each game
                for(int i=0;i<fw_update_num;i++){
                    lv_obj_t * update_btn = lv_list_add_btn(list_fw_update, NULL, fw_update_name[i]);
                    lv_group_add_obj(group_interact, update_btn);
                    lv_obj_set_event_cb(update_btn, fw_update_cb);
                }
                lv_group_add_obj(group_interact, list_fw_update);

                lv_group_focus_obj(list_fw_update);
            }
            else{
            /* lv_obj_del(container_header_game_icon);

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
                lv_obj_set_click(lv_layer_top(), true);*/
            }
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Brightness")==0){
            mbox_brightness = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_brightness, "Brightness");
            
            lv_group_add_obj(group_interact, mbox_brightness);
            lv_group_focus_obj(mbox_brightness);

            lv_obj_t * slider = lv_slider_create(mbox_brightness, NULL);
            lv_obj_align(slider, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_width(slider, 180);
            lv_slider_set_range(slider, 0, 100);
            
            uint8_t brightness_level = backlight_get();
            lv_slider_set_value(slider,brightness_level,LV_ANIM_ON);

            lv_obj_set_event_cb(slider, slider_brightness_cb);

            lv_group_add_obj(group_interact, slider);
            lv_group_focus_obj(slider);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"GUI Color Mode")==0){

            if(GUI_THEME == LV_THEME_MATERIAL_FLAG_LIGHT){
                system_save_config(SYS_GUI_COLOR,1);
                GUI_theme_color(1);
            }
            else{
                system_save_config(SYS_GUI_COLOR,0);
                GUI_theme_color(0);
            }
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Volume")==0){
            mbox_volume = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_volume, "Volume");
            
            lv_group_add_obj(group_interact, mbox_volume);
            lv_group_focus_obj(mbox_volume);

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
        else if(strcmp(lv_list_get_btn_text(parent),"Enable Button State Save")==0){
            lv_obj_t * label1 = lv_list_get_btn_label(parent);
            lv_label_set_text(label1,"Disable Button State Save");
            system_save_config(SYS_STATE_SAV_BTN,1);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Disable Button State Save")==0){
            lv_obj_t * label1 = lv_list_get_btn_label(parent);
            lv_label_set_text(label1,"Enable Button State Save");
            system_save_config(SYS_STATE_SAV_BTN,0);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"Battery Status")==0){
            //Create message box
            lv_obj_t * mbox_battery = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_battery, "Battery Status");
            lv_obj_align(mbox_battery, NULL, LV_ALIGN_CENTER, 0, -50);

            // Get the battery data and print with bullet points
            char spec_text[256];
            sprintf(spec_text,"\u2022 Total Capacity: %imAh\n\u2022 Voltage: %imV",\
            500,management.voltage);
            lv_obj_t * lable_battery_info = lv_label_create(mbox_battery, NULL);
            lv_label_set_text(lable_battery_info,spec_text);

            // Bar which show the actual capacity of the battery
            lv_obj_t * bar_battery = lv_bar_create(mbox_battery, NULL);
            lv_obj_set_size(bar_battery, 200, 20);
            lv_obj_align(bar_battery, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_bar_set_anim_time(bar_battery, 2000);
            
            
            if(management.voltage >= 4165){
                // Connected to the USB
                // TODO: Change the possition of the text or if it's possible create an animation with the numbers
                lv_obj_t * label_battery_bar = lv_label_create(bar_battery, NULL);
                lv_label_set_text(label_battery_bar,"Charged");
                lv_obj_align(label_battery_bar, NULL, LV_ALIGN_CENTER, 5, 0);

                lv_obj_set_style_local_bg_color(bar_battery, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x25CDF6));
                lv_bar_set_value(bar_battery, 100, LV_ANIM_ON);
            }
            else{
                lv_obj_t * label_battery_bar = lv_label_create(bar_battery, NULL);
                char battery_level[4];
                sprintf(battery_level,"%i",management.percentage);
                lv_label_set_text(label_battery_bar,battery_level);
                lv_obj_align(label_battery_bar, NULL, LV_ALIGN_CENTER, 5, 0);

                lv_obj_set_style_local_bg_color(bar_battery, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x0CC62D));
                lv_bar_set_value(bar_battery, management.percentage, LV_ANIM_ON);

            }
            lv_obj_set_event_cb(mbox_battery, mbox_config_cb);
            lv_group_add_obj(group_interact, mbox_battery);
            lv_group_focus_obj(mbox_battery);
        }
        else if(strcmp(lv_list_get_btn_text(parent),"SD card Status")==0){
            lv_obj_t * mbox_SD = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox_SD, "SD Card Information");
            lv_obj_align(mbox_SD, NULL, LV_ALIGN_CENTER, 0, -50);

            if(sd_mounted()){
                lv_obj_t * label1 = lv_label_create(mbox_SD, NULL);
                char spec_text[256];
                // TODO: The SD card type is wrong, it can't show all the types
                sprintf(spec_text,"\u2022 SD Name: %s\n\u2022 Size: %i MB \n\u2022 Speed: %i Khz\n\u2022 Type: %s" \
                ,sd_card_info.card_name, sd_card_info.card_size, sd_card_info.card_speed,\
                (sd_card_info.card_type & SDIO) ? "SDIO" : "SDHC/SDXC");
                lv_label_set_text(label1,spec_text);
                lv_obj_set_event_cb(mbox_SD, mbox_config_cb);
            }
            else{
                lv_obj_t * label1 = lv_label_create(mbox_SD, NULL);
                lv_label_set_text(label1,"SD card not available");
                lv_obj_set_event_cb(mbox_SD, mbox_config_cb);
            }

            lv_group_add_obj(group_interact, mbox_SD);
            lv_group_focus_obj(mbox_SD);
            
        }
    }
    else if(e == LV_EVENT_CANCEL){
        sub_menu = false;
        lv_obj_del(list_config);
        lv_group_focus_obj(config_btn);
    }

}

static void fw_update_cb(lv_obj_t * parent, lv_event_t e){

    if(e == LV_EVENT_CLICKED){

        ESP_LOGI(TAG, "FW update: %s",(char *)lv_list_get_btn_text(parent));
        
        lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
        lv_msgbox_set_text(mbox, "Firmware update:");
        lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, -60);

        lv_obj_t * label1 = lv_label_create(mbox, NULL);
        lv_label_set_text(label1,lv_list_get_btn_text(parent));

        lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
        lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_set_click(lv_layer_top(), true);

        lv_group_focus_obj(mbox);
        lv_group_focus_freeze(group_interact, false);

        lv_obj_t * preload = lv_spinner_create(mbox, NULL);
        lv_obj_set_size(preload, 100, 100);
        lv_obj_align(preload, NULL, LV_ALIGN_CENTER, 0, 0);

        struct SYSTEM_MODE fw_update;
        fw_update.mode = MODE_UPDATE;
        strcpy(fw_update.game_name, (char *)lv_list_get_btn_text(parent));

        if( xQueueSend(modeQueue,&fw_update, ( TickType_t ) 10) != pdPASS ){
            ESP_LOGE(TAG, "Error sending update execution queue");
        }
        
    }
    else if(e == LV_EVENT_CANCEL){
        lv_obj_del(list_fw_update);
    } 
}

static void mbox_config_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CANCEL){
        lv_obj_del(parent);
    }  
}

static void GUI_theme_color(uint8_t color_selected){
    if(color_selected) GUI_THEME = LV_THEME_MATERIAL_FLAG_DARK;
    else GUI_THEME = LV_THEME_MATERIAL_FLAG_LIGHT;

    LV_THEME_DEFAULT_INIT(lv_theme_get_color_primary(), lv_theme_get_color_secondary(),
    GUI_THEME,
    lv_theme_get_font_small(), lv_theme_get_font_normal(), lv_theme_get_font_subtitle(), lv_theme_get_font_title());
}

/****** External Async functions ***********/
void async_battery_alert(bool game_mode){

    lv_obj_t * mbox_battery_Alert = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox_battery_Alert, "Battery Alert");

    lv_obj_t * mbox_battery = lv_label_create(mbox_battery_Alert, NULL);
    lv_label_set_text(mbox_battery,"Battery level below 10%");
    lv_obj_align(mbox_battery, NULL, LV_ALIGN_CENTER, -50, 0);

    static const char * btns[] = {"Ok", "", ""};
    lv_msgbox_add_btns(mbox_battery_Alert, btns);

    lv_group_add_obj(group_interact, mbox_battery_Alert);
    lv_group_focus_obj(mbox_battery_Alert);



}
/**********************
 *   TASKS FUNCTIONS
 **********************/

static void battery_status_task(lv_task_t * task){
    // This task check every minute if the battery level has change. If it receive a message from the 
    // queue, it change the text value and the size of the battery bar.

    if(xQueueReceive(batteryQueue, &management,( TickType_t ) 0)){

        if(management.voltage >= 4190){

            lv_obj_set_hidden(Charging_label,false); //Show the icon
            lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x25CDF6)); // Change the color to blue

            lv_bar_set_value(battery_bar, 100, NULL);
            
            //TODO: Improve USB text positition
            lv_label_set_text(battery_label,"USB");

        }
        else{
            lv_obj_set_hidden(Charging_label,true);
            if(management.percentage == 1){
                lv_label_set_text(battery_label, LV_SYMBOL_WARNING);
                lv_bar_set_value(battery_bar, 100, NULL);
            }
            else{
                 char *battery_level = malloc(4); 
                sprintf(battery_level,"%i",management.percentage);
                lv_label_set_text(battery_label, battery_level);
                lv_bar_set_value(battery_bar, management.percentage, NULL);
                free(battery_level);
            }
            lv_obj_align(battery_label, NULL, LV_ALIGN_CENTER, 0, 0);

            // If the battery is equal or less than the 25% the bar change to red, otherwise the bar is green.
            if(management.percentage <= 25){
                lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0xC61D0C));
            }
            else{
                lv_obj_set_style_local_bg_color(battery_bar, LV_BAR_PART_INDIC, LV_STATE_DEFAULT, lv_color_hex(0x0CC62D));
            }

            
        }  
    }
}

static bool user_input_task(lv_indev_drv_t * indev_drv, lv_indev_data_t * data){

    // Get the status of the multiplexer driver
    uint16_t inputs_value =  input_read();

    if(!((inputs_value >> 0) & 0x01)){
        // Button Down pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        // To avoid bounce with the buttons, we need to wait 2 ms.
        if((actual_time-btn_down_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_DOWN;

            // Save the actual time to calculate the bounce time.
            btn_down_time = actual_time;
        }
    }

    if(!((inputs_value >> 1) & 0x01)){
        // Button Left pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;
        if(sub_menu){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_LEFT;
            btn_right_time = actual_time;
        }
        else{
            if((actual_time-btn_right_time) > bouncing_time){
                if(tab_num > 0 ){
                    tab_num--;
                    data->state = LV_INDEV_STATE_PR;
                    data->key = LV_KEY_PREV;
                    lv_tabview_set_tab_act(tab_main_menu, tab_num, LV_ANIM_ON);
                }
                btn_right_time = actual_time;
            }
        } 
    }

    if(!((inputs_value >> 2) & 0x01)){
        // Button up pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;
        if((actual_time-btn_up_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_UP;
            btn_up_time = actual_time;
        }
         
    }

    if(!((inputs_value >> 3) & 0x01)){
        // Button right pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if(sub_menu){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_RIGHT;
            btn_right_time = actual_time;
        }
        else{
            if((actual_time-btn_right_time) > bouncing_time){
                if(tab_num < 2){
                    tab_num++;
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
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_menu_time) > bouncing_time){
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
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_b_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_ESC;
            btn_b_time = actual_time;
        }
    }

    if(!((inputs_value >> 9) & 0x01)){
        // Button A pushed
        uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;

        if((actual_time-btn_a_time) > bouncing_time){
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_ENTER;
            btn_a_time = actual_time;
        }
    }

    return false;
}
