
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_eth.h"

#include <dirent.h>

#include <esp_http_server.h>

#include "ext_flash.h"

/* A simple example that demonstrates how to create GET and POST
 * handlers for the web server.
 */

static const char *TAG = "example";

char file_incoming_buffer[8192];

static esp_err_t gamelist_get_handler(httpd_req_t *req){
    struct dirent *entry;
    const char *entrytype;
    char entrypath[200];
    char entrysize[16];
    struct stat entry_stat;
    char file_buffer[1024];
    memset(file_buffer,0,sizeof(file_buffer));
    
    DIR *dir = opendir("/ext_flash/");
    const size_t dirpath_len = strlen("/ext_flash/");

    strlcpy(entrypath, "/ext_flash/", sizeof(entrypath));

    if (!dir) {
        ESP_LOGE(TAG, "Failed to stat dir : %s", "/ext_flash/");
        /* Respond with 404 Not Found */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Directory does not exist");
        return ESP_FAIL;
    }
    int i =0;
    while((entry = readdir(dir)) != NULL){
        char temp_name_buffer[300];
        entrytype = (entry->d_type == DT_DIR ? "directory" : "file");

        strlcpy(entrypath + dirpath_len, entry->d_name, sizeof(entrypath) - dirpath_len);
        if (stat(entrypath, &entry_stat) == -1) {
            ESP_LOGE(TAG, "Failed to stat %s : %s", entrytype, entry->d_name);
            continue;
        }
        sprintf(entrysize, "%ld", entry_stat.st_size);
        sprintf(temp_name_buffer,"%s/%s/",entry->d_name,entrysize);
        strncat(file_buffer,temp_name_buffer,strlen(temp_name_buffer));
        printf("%s\r\n",file_buffer);
        ESP_LOGI(TAG, "Found %s : %s (%s bytes)", entrytype, entry->d_name, entrysize);
    }
    
    httpd_resp_set_type(req, "text/plain");
    char ussage[3] = {0};
    sprintf(ussage,"%i",ext_flash_ussage());
    
    httpd_resp_send(req, file_buffer, strlen(file_buffer));

    closedir(dir);
    return ESP_OK;
}

static esp_err_t web_download(httpd_req_t *req){

    extern const unsigned char main_html_start[] asm("_binary_main_html_start");
    extern const unsigned char main_html_end[]   asm("_binary_main_html_end");
    const size_t main_html_size = (main_html_end - main_html_start);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)main_html_start, main_html_size);

    return ESP_OK;
}

static esp_err_t w3css_get_handler(httpd_req_t *req)
{
    extern const unsigned char upload_script_start0[] asm("_binary_w3_css_start");
    extern const unsigned char upload_script_end0[]   asm("_binary_w3_css_end");
    const size_t upload_script_size0 = (upload_script_end0 - upload_script_start0);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)upload_script_start0, upload_script_size0);
    return ESP_OK;
}

static esp_err_t memory_ussage_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/plain");
    char ussage[3] = {0};
    sprintf(ussage,"%i",ext_flash_ussage());
    
    httpd_resp_send(req, ussage, strlen(ussage));
    return ESP_OK;
}

static esp_err_t upload_post_handler(httpd_req_t *req){

    char *uri_file_path,*results;
    char file_name[30];
    char file_path[100];

    FILE *fd = NULL;

    memset(file_name,'0',sizeof(file_name));
    memset(file_path,'0',sizeof(file_path)); 

    uri_file_path = strdup(req->uri);
 
    // Loop to get the file name
    while( (results = strsep(&uri_file_path,"/")) != NULL ){
        if(strncmp(results,"upload",7)!=0){
            strcpy (file_name,results);
        }
    }
    
    // Check if the size is to big

    // Create file to save the incoming data
    sprintf(file_path,"/ext_flash/%s",file_name);
    
    fd = fopen(file_path, "wb+");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", file_path);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving file : %s...", file_name);

    char *temp_buf;
    uint32_t download_size = req->content_len;
    printf("File size %i\r\n",download_size);
    uint32_t download_received = 0;

    while(download_size>0){
        
        ESP_LOGI(TAG, "Remaining size : %d", 100-((download_size*100)/req->content_len));
        if ((download_received = httpd_req_recv(req, file_incoming_buffer, MIN(download_size, 8192))) <= 0) {
            if (download_received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error,
             * close and delete the unfinished file*/
            fclose(fd);
            unlink(file_path);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (download_received && (download_received != fwrite(file_incoming_buffer, 1, download_received, fd))) {
            /* Couldn't write everything to file!
             * Storage may be full? */
            fclose(fd);
            unlink(file_path);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of
         * the file left to be uploaded */
        download_size -= download_received;

    }

    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");

    printf("Todo OK\r\n");

    return ESP_OK;
}

static esp_err_t delete_post_handler(httpd_req_t *req){

    char *uri_file_path,*results;
    char file_name[30];
    char file_path[100];
    struct stat file_stat;

    FILE *fd = NULL;

    memset(file_name,'0',sizeof(file_name));
    memset(file_path,'0',sizeof(file_path)); 

    uri_file_path = strdup(req->uri);
 
    // Loop to get the file name
    while( (results = strsep(&uri_file_path,"/")) != NULL ){
        if(strncmp(results,"upload",7)!=0){
            strcpy (file_name,results);
        }
    }

    // Create file to save the incoming data
    sprintf(file_path,"/ext_flash/%s",file_name);
    printf("FILEpath delete: %s\r\n",file_path);

    if (stat(file_path, &file_stat) == -1) {
        ESP_LOGE(TAG, "File does not exist : %s", file_name);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File does not exist");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Deleting file : %s", file_name);
    /* Delete file */
    unlink(file_path);

    /* Redirect onto root to see the updated file list */
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File deleted successfully");

    return ESP_OK;
}


esp_err_t start_webserver(){

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    config.uri_match_fn = httpd_uri_match_wildcard;

    if(httpd_start(&server, &config) != ESP_OK){
        ESP_LOGI(TAG, "Server start fail");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Server Started");

    static const httpd_uri_t web_download_handler = {
        .uri       = "/",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = web_download,
        .user_ctx  = NULL    // Pass server data as context
    };
    httpd_register_uri_handler(server, &web_download_handler);

    static const httpd_uri_t w3_css_download_handler = {
        .uri       = "/w3.css",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = w3css_get_handler,
        .user_ctx  = NULL    // Pass server data as context
    };
    httpd_register_uri_handler(server, &w3_css_download_handler);

    static const httpd_uri_t mem_ussage_handler = {
        .uri       = "/mem_ussage",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = memory_ussage_get_handler,
        .user_ctx  = NULL    // Pass server data as context
    };
    httpd_register_uri_handler(server, &mem_ussage_handler);

    static const httpd_uri_t upload_handler = {
        .uri       = "/upload/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = file_incoming_buffer    
    };
    httpd_register_uri_handler(server, &upload_handler);

    static const httpd_uri_t gamelist_handler = {
        .uri       = "/gamelist",  // Match all URIs of type /path/to/file
        .method    = HTTP_GET,
        .handler   = gamelist_get_handler,
        .user_ctx  = NULL    
    };
    httpd_register_uri_handler(server, &gamelist_handler);

    static const httpd_uri_t delete_handler = {
        .uri       = "/delete/*",  // Match all URIs of type /path/to/file
        .method    = HTTP_POST,
        .handler   = delete_post_handler,
        .user_ctx  = NULL    
    };
    httpd_register_uri_handler(server, &delete_handler);

    return ESP_OK;
}



/*static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}*/