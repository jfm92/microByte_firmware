#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include "freertos/queue.h"
#include "GUI.h"
#include "ext_flash.h"
#include <loader.h>
#include "lvgl.h"
/*********************
 *      DEFINES
 *********************/
#define BTN_UP          34
#define BTN_DOWN        35
#define BTN_RIGHT       25
#define BTN_LEFT        32
#define BTN_A           26
#define BTN_B           12
#define BTN_START       2
#define BTN_SELECT      15

/**********************
 *  STATIC PROTOTYPES
 **********************/

static bool keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data);

static void game_menu(lv_obj_t * parent);
static void upload_menu(lv_obj_t * parent);
static void config_menu(lv_obj_t * parent);

static void tab_manager_cb(lv_obj_t * ta, lv_event_t e);
//LV_EVENT_CB_DECLARE(tab_manager_cb);

static void game_menu_event_cb(lv_obj_t * parent, lv_event_t e);
static void game_list_event(lv_obj_t * parent, lv_event_t e);
static void game_execute_cb(lv_obj_t * parent, lv_event_t e);
static void msgbox_event_cb(lv_obj_t * msgbox, lv_event_t e);
static void upload_event_cb(lv_obj_t * parent, lv_event_t e);

/**********************
 *  GLOBAL VARIABLES
 **********************/
int tab_num = 0;
uint32_t btn_left_time = 0;
uint32_t btn_right_time = 0;

//Temporary

char wifi_name[256] = {"foo_123g"};
char wifi_password[256] = {"12345"};
char wifi_dir[256] = {"192.168.1.37"};

uint8_t battery_level = 100;


/**********************
 *  STATIC VARIABLES
 **********************/

static lv_group_t * group_kp;

static lv_obj_t * tab_manager;
static lv_obj_t * tab_gamelist;
static lv_obj_t * tab_upload;
static lv_obj_t * tab_config;

static lv_obj_t * notification_cont;
static lv_obj_t * battery_bar;

static lv_obj_t * game_lib_btn;
static lv_obj_t * add_game_btn;
static lv_obj_t * config_btn;

static lv_obj_t * game_list;

static lv_indev_t * kb_indev;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void GUI_menu(){
    // Create object group which can be manipulated by the keypad
    group_kp = lv_group_create();

    //Initialize Keypad
    lv_indev_drv_t kb_drv;
    lv_indev_drv_init(&kb_drv);
    kb_drv.type = LV_INDEV_TYPE_KEYPAD;
    kb_drv.read_cb = keyboard_read;
    kb_indev = lv_indev_drv_register(&kb_drv);
    lv_indev_set_group(kb_indev, group_kp);

    //Notifications top bar
    /*notification_cont = lv_cont_create(lv_scr_act(), NULL);
    lv_obj_align_origo(notification_cont, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);
    lv_obj_set_size(notification_cont,240,50);
    lv_cont_set_fit(notification_cont, LV_FIT_TIGHT);
    lv_cont_set_layout(notification_cont, LV_LAYOUT_COL_MID);*/

    //Create menu tabs
    tab_manager = lv_tabview_create(lv_scr_act(),NULL);
    lv_tabview_set_btns_pos(tab_manager,LV_TABVIEW_TAB_POS_NONE);

    tab_gamelist = lv_tabview_add_tab(tab_manager,"Game List");
    tab_upload = lv_tabview_add_tab(tab_manager,"Upload");
    tab_config = lv_tabview_add_tab(tab_manager,"Configuration");

    game_menu(tab_gamelist);
    upload_menu(tab_upload);
    config_menu(tab_config);

}

static void game_menu(lv_obj_t * parent){
    // TODO add image as button
    game_lib_btn = lv_btn_create(parent, NULL);
    lv_obj_align(game_lib_btn, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t * label = lv_label_create(game_lib_btn, NULL);
    lv_label_set_text(label, "Game Library");

    lv_obj_set_event_cb(game_lib_btn, game_menu_event_cb); 
    lv_group_add_obj(group_kp, game_lib_btn);
    
}

static void upload_menu(lv_obj_t * parent){
    // TODO add image as button
    add_game_btn = lv_btn_create(parent, NULL);
    lv_obj_align(add_game_btn, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_t * label = lv_label_create(add_game_btn, NULL);
    lv_label_set_text(label, "Add a game");

    lv_obj_set_event_cb(add_game_btn, upload_event_cb); 
    lv_group_add_obj(group_kp, add_game_btn);
}

static void config_menu(lv_obj_t * parent){
    config_btn = lv_btn_create(parent, NULL);

   lv_obj_t * label = lv_label_create(config_btn, NULL);
   lv_label_set_text(label, "Configuration");

   //lv_obj_set_event_cb(selector_objs.btn, gl_btn_event); 
   lv_group_add_obj(group_kp, config_btn);
}


static void game_menu_event_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED){
        //Get the games names and how many are available
        char game_names[30][100];
        uint8_t games_num = ext_flash_game_list(game_names);
 
        if(games_num>0){
            // Create a list with the name of the games
            game_list = lv_list_create(lv_layer_top(), NULL);
            lv_obj_set_size(game_list, 230, 180);
            lv_obj_align(game_list, NULL, LV_ALIGN_CENTER, 0, 0);
            lv_obj_set_event_cb(game_list, game_list_event);
            
            // Add a button for each game
            for(int i=0;i<games_num;i++){
                lv_obj_t * game_btn = lv_list_add_btn(game_list, LV_SYMBOL_VIDEO, game_names[i]);
                lv_obj_set_event_cb(game_btn, game_execute_cb);
            }
            lv_group_add_obj(group_kp, game_list);
            lv_group_focus_obj(game_list);
        }
        else{
            // Show a message informs any game is available
            lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
            lv_msgbox_set_text(mbox, "Any game available.\n Please, upload a new one!");
            lv_obj_set_event_cb(mbox, msgbox_event_cb);
            lv_group_add_obj(group_kp, mbox);
            lv_group_focus_obj(mbox);
            lv_group_focus_freeze(group_kp, true);

            static const char * btns[] = {"Ok", "", ""};
            lv_msgbox_add_btns(mbox, btns);
            lv_obj_align(mbox, NULL, LV_ALIGN_CENTER, 0, 0);

            lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
            lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
            lv_obj_set_click(lv_layer_top(), true);
        }
        
        
    }

}

static void msgbox_event_cb(lv_obj_t * msgbox, lv_event_t e)
{
    if(e == LV_EVENT_CLICKED) {
        uint16_t b = lv_msgbox_get_active_btn(msgbox);
        if(b == 0 || b == 1) {
            lv_obj_del(msgbox);
            lv_obj_reset_style_list(lv_layer_top(), LV_OBJ_PART_MAIN);
            lv_obj_set_click(lv_layer_top(), false);
            lv_group_focus_obj(game_lib_btn);
        }
    }
}

static void game_execute_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED) {

        if( xQueueSend( loader_name,(char *)lv_list_get_btn_text(parent) , ( TickType_t ) 10) != pdPASS )
        {
            printf("Failed to send\r\n");
        }
        char msg_box_txt[200]={0};
        sprintf(msg_box_txt,"Loading: %s",lv_list_get_btn_text(parent));
        lv_obj_t * mbox = lv_msgbox_create(lv_layer_top(), NULL);
        lv_msgbox_set_text(mbox, msg_box_txt);
        lv_obj_set_event_cb(mbox, msgbox_event_cb);
        lv_group_add_obj(group_kp, mbox);
        lv_group_focus_obj(mbox);
        lv_obj_del(game_list);
        lv_group_focus_freeze(group_kp, true);
        lv_obj_set_style_local_bg_opa(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_OPA_70);
        lv_obj_set_style_local_bg_color(lv_layer_top(), LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_GRAY);
        lv_obj_set_click(lv_layer_top(), true);

        
    }
}

static void upload_event_cb(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CLICKED) {
        //Start wi-fi ap server
        lv_obj_t * dc_cont;

        dc_cont = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_auto_realign(dc_cont, true);                    /*Auto realign when the size changes*/
        lv_obj_align_origo(dc_cont, NULL, LV_ALIGN_CENTER, 0, 0);  /*This parametrs will be sued when realigned*/
        lv_cont_set_fit(dc_cont, LV_FIT_TIGHT);
        lv_cont_set_layout(dc_cont, LV_LAYOUT_COLUMN_MID);

        lv_obj_t * label;
        label = lv_label_create(dc_cont, NULL);
        lv_label_set_text(label, "Wi-Fi AP data:");
        lv_obj_align_origo(label, NULL, LV_ALIGN_IN_TOP_MID, 0, 0);

        lv_obj_t * table = lv_table_create(lv_scr_act(), NULL);
        lv_table_set_col_cnt(table, 2);
        lv_table_set_row_cnt(table, 3);
        lv_obj_align(table, NULL, LV_ALIGN_CENTER, 0, 0);

        lv_table_set_cell_value(table, 0, 0, "Wi-Fi AP");
        lv_table_set_cell_value(table, 1, 0, "Password");
        lv_table_set_cell_value(table, 2, 0, "IP");

        lv_table_set_cell_value(table, 0, 1, wifi_name);
        lv_table_set_cell_value(table, 1, 1, wifi_password);
        lv_table_set_cell_value(table, 2, 1, wifi_dir);
    }
    
}

static void game_list_event(lv_obj_t * parent, lv_event_t e){
    if(e == LV_EVENT_CANCEL  ){
        lv_obj_del(game_list);
        lv_group_focus_obj(game_lib_btn);
    }
}

uint8_t button_read(uint8_t btn){
    uint32_t actual_time= xTaskGetTickCount()/portTICK_PERIOD_MS;
    //printf("Actual_time %i\r\n",actual_time);
    if(btn==BTN_RIGHT && (actual_time-btn_right_time)>5 && gpio_get_level(btn)){
        btn_right_time=actual_time;
        return 1;
    }
    else if(btn==BTN_LEFT && (actual_time-btn_left_time)>5 && gpio_get_level(btn)){
        btn_left_time=actual_time;
        return 1;
    }
    return 0;
}

static bool keyboard_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data)
{

    if(gpio_get_level(BTN_UP)){
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_KEY_UP;
    }
    else if(gpio_get_level(BTN_DOWN)){
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_KEY_DOWN;
    }
    else if(button_read(BTN_LEFT)){
        tab_num--;
        if(tab_num<0){
            tab_num=0;
           // data->key = LV_KEY_NEXT;
        }
        else{
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_PREV;
        }
        lv_tabview_set_tab_act(tab_manager, tab_num, LV_ANIM_ON);
        

    }
     else if(button_read(BTN_RIGHT)){
        tab_num++;
        if(tab_num>2){
            tab_num=2;
           // data->key = LV_KEY_PREV;
        }
        else{
            data->state = LV_INDEV_STATE_PR;
            data->key = LV_KEY_NEXT;
        }
        lv_tabview_set_tab_act(tab_manager, tab_num, LV_ANIM_ON);
        
    }
    else if(gpio_get_level(BTN_A)){
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_KEY_ENTER;
    }
    else if(gpio_get_level(BTN_START)){
        data->state = LV_INDEV_STATE_PR;
        data->key = LV_KEY_ESC;
    }

    return false;
}