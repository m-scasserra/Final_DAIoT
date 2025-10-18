#include "http_server.h"

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_http_server.h>
#include <stdio.h>
#include <ctype.h>

#include "esp_idf_version.h"
#include "esp_mac.h"
#include "cJSON.h"
#include "esp_chip_info.h"

static const char *TAG = "HTTPServer";

extern const uint8_t _binary_html_page_index_html_start[];
extern const uint8_t _binary_html_page_index_html_end[];

extern const uint8_t _binary_html_page_device_data_html_start[];
extern const uint8_t _binary_html_page_device_data_html_end[];

extern const uint8_t _binary_html_page_wifi_config_html_start[];
extern const uint8_t _binary_html_page_wifi_config_html_end[];

httpd_handle_t server = NULL;
httpd_config_t config = HTTPD_DEFAULT_CONFIG();

esp_timer_handle_t restart_timer;

wifi_credentials_t wifi_credentials;
device_data_t device_data_info;

static void start_webserver(void);
static void stop_webserver(void);
static void set_wifi_credentials_received_event_callback(void *);
void (*event_recived_wifi_conf_callback)(void);

void preprocess_string(char *str)
{
    char *p, *q;

    for (p = q = str; *p != 0; p++)
    {
        if (*(p) == '%' && *(p + 1) != 0 && *(p + 2) != 0)
        {
            // quoted hex
            uint8_t a;
            p++;
            if (*p <= '9')
                a = *p - '0';
            else
                a = toupper((unsigned char)*p) - 'A' + 10;
            a <<= 4;
            p++;
            if (*p <= '9')
                a += *p - '0';
            else
                a += toupper((unsigned char)*p) - 'A' + 10;
            *q++ = a;
        }
        else if (*(p) == '+')
        {
            *q++ = ' ';
        }
        else
        {
            *q++ = *p;
        }
    }
    *q = '\0';
}

static void restart_timer_callback(void *arg)
{
    ESP_LOGI(TAG, "Restarting now...");
    esp_restart();
}

esp_timer_create_args_t restart_timer_args = {
    .callback = &restart_timer_callback,
    /* argument specified here will be passed to timer callback function */
    .arg = (void *)0,
    .name = "restart_timer"};

/* HTTP GET handler (generic) */
static esp_err_t index_get_handler(httpd_req_t *req)
{
    char *buf;
    size_t buf_len;

    /* Get header value string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_hdr_value_len(req, "Host") + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        /* Copy null terminated value string into buffer */
        if (httpd_req_get_hdr_value_str(req, "Host", buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found header => Host: %s", buf);
        }
        free(buf);
    }

    /* Read URL query string length and allocate memory for length + 1,
     * extra byte for null termination */
    buf_len = httpd_req_get_url_query_len(req) + 1;
    if (buf_len > 1)
    {
        buf = malloc(buf_len);
        if (httpd_req_get_url_query_str(req, buf, buf_len) == ESP_OK)
        {
            ESP_LOGI(TAG, "Found URL query => %s", buf);
            if (strcmp(buf, "reset=Reboot") == 0)
            {
                esp_timer_start_once(restart_timer, 500000);
            }

            /* Get value of expected key from query string */
            if (httpd_query_key_value(buf, "ssid", wifi_credentials.ssid, sizeof(wifi_credentials.ssid)) == ESP_OK)
            {
                ESP_LOGI(TAG, "Found URL query parameter => ssid=%s", wifi_credentials.ssid);
                preprocess_string(wifi_credentials.ssid);
                if (httpd_query_key_value(buf, "password", wifi_credentials.pass, sizeof(wifi_credentials.pass)) == ESP_OK)
                {
                    ESP_LOGI(TAG, "Found URL query parameter => password=%s", wifi_credentials.pass);
                    preprocess_string(wifi_credentials.pass);
                    if (event_recived_wifi_conf_callback != NULL)
                        event_recived_wifi_conf_callback();
                }
            }
        }
        free(buf);
    }

    /* Send response with custom headers and body set as the
     * string passed in user context*/
    const char *resp_str = (const char *)req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));
    return ESP_OK;
}

static httpd_uri_t indexp =
    {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_get_handler,
};

static httpd_uri_t wifi_settings =
    {
        .uri = "/wifi_settings",
        .method = HTTP_GET,
        .handler = index_get_handler,
};

static httpd_uri_t device_data =
    {
        .uri = "/device_data",
        .method = HTTP_GET,
        .handler = index_get_handler,
};

static httpd_uri_t device_data_content =
    {
        .uri = "/device_data_content",
        .method = HTTP_GET,
        .handler = index_get_handler,
};

esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Page not found");
    return ESP_FAIL;
}

char *param_set_default(const char *def_val)
{
    char *retval = malloc(strlen(def_val) + 1);
    strcpy(retval, def_val);
    return retval;
}

static void load_device_data_info(void)
{

    // ESP-IDF Version
    ESP_LOGW(TAG, "version %s", esp_get_idf_version());
    sprintf(device_data_info.esp_idf_version, "%s", esp_get_idf_version());

    // ESP-IDF Mac
    uint8_t mac[10];

    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    ESP_LOGW(TAG, "Wi-Fi STA Mac: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(device_data_info.wifi_sta_mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    ESP_LOGW(TAG, "Wi-Fi AP Mac: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(device_data_info.wifi_ap_mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_read_mac(mac, ESP_MAC_BT);
    ESP_LOGW(TAG, "Wi-Fi BT Mac: %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    sprintf(device_data_info.wifi_bt_mac, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    sprintf(device_data_info.esp_cores, "%d", chip_info.cores);
}

static void load_server_static_content(void)
{

    // const char *index_page_template = html_page_index_html;
    indexp.user_ctx = (void *)_binary_html_page_index_html_start;
}

static void load_server_dinamic_content(void)
{
    char *wifi_settings_page = malloc(strlen((const char *)_binary_html_page_wifi_config_html_start) + 512);
    if (strlen(wifi_credentials.ssid) == 0)
        sprintf(wifi_settings_page, (const char *)_binary_html_page_wifi_config_html_start, "SSID of the new network", "Password of the new network");
    else
    {
        ESP_LOGI(TAG, "SSID data refresh...");
        sprintf(wifi_settings_page, (const char *)_binary_html_page_wifi_config_html_start, wifi_credentials.ssid, "******");
    }

    httpd_unregister_uri(server, "/wifi_settings");
    wifi_settings.user_ctx = wifi_settings_page;
    httpd_register_uri_handler(server, &wifi_settings);

    device_data.user_ctx = (void *)_binary_html_page_device_data_html_start;

    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "idf_version", esp_get_idf_version());
    cJSON_AddStringToObject(root, "esp_cores", device_data_info.esp_cores);
    cJSON_AddStringToObject(root, "wifi_sta_mac", device_data_info.wifi_sta_mac);
    cJSON_AddStringToObject(root, "wifi_ap_mac", device_data_info.wifi_ap_mac);
    cJSON_AddStringToObject(root, "bt_mac", device_data_info.wifi_bt_mac);
    cJSON_AddStringToObject(root, "wifi_sta_ip", device_data_info.wifi_sta_ip);
    const char *sys_info = cJSON_Print(root);

    ESP_LOGW(TAG, "JSON Device Data %s", sys_info);

    httpd_unregister_uri(server, "/device_data_content");
    device_data_content.user_ctx = (void *)sys_info;
    httpd_register_uri_handler(server, &device_data_content);

    // free((void *)sys_info);
    cJSON_Delete(root);
}

void start_webserver(void)
{

    esp_timer_create(&restart_timer_args, &restart_timer);

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        load_server_static_content();

        load_device_data_info();

        load_server_dinamic_content();
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &indexp);
        httpd_register_uri_handler(server, &wifi_settings);
        httpd_register_uri_handler(server, &device_data);
        // httpd_register_uri_handler(server, &device_data_content);
        return;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return;
}

static void stop_webserver(void)
{
    // Stop the httpd server
    httpd_stop(server);
}

static void set_wifi_credentials_received_event_callback(void *callback)
{
    ESP_LOGI(TAG, "Setting WiFi credentials event callback.");
    event_recived_wifi_conf_callback = callback;
}

static void set_ssid_to_show_on_config_page(char *ssid)
{
    if (ssid != NULL)
    {
        strcpy(wifi_credentials.ssid, ssid);
        load_server_dinamic_content();
    }
}

static void set_sta_ip_to_show_on_config_page(char *ip_string)
{

    sprintf(device_data_info.wifi_sta_ip, "%s", ip_string);
    load_server_dinamic_content();
}

/*****************************************************
 *   Driver Instance Declaration(s) API(s)            *
 ******************************************************/
const webserver_t webserver = {
    // WebServer Props
    .wifi_credentials = &wifi_credentials,
    // WebServer Functions
    .start_webserver = start_webserver,
    .stop_webserver = stop_webserver,
    .set_wifi_credentials_received_event_callback = set_wifi_credentials_received_event_callback,
    .set_ssid_to_show_on_config_page = set_ssid_to_show_on_config_page,
    .set_sta_ip_to_show_on_config_page = set_sta_ip_to_show_on_config_page,
};