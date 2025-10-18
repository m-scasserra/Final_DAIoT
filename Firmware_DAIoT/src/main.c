#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "wifi_manager.h"
#include "http_server.h"
#include "mqtt_manager.h"
#include "led.h"

static const char *TAG = "Main section";

void wifi_credentials_received_callback(void)
{
    ESP_LOGI(TAG, "Wi-Fi credentials received from WebServer...");
    ESP_LOGI(TAG, "ssid: %s / pass: %s", webserver.wifi_credentials->ssid, webserver.wifi_credentials->pass);
    wifi_manager.set_sta_credentials(webserver.wifi_credentials->ssid, webserver.wifi_credentials->pass);
}

void wifi_got_ip_event_callback(void)
{
    webserver.set_sta_ip_to_show_on_config_page(wifi_manager.get_sta_ip());
}

void app_main(void)
{
    // Initialize LED
    begin_led();
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Default Event Loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize Wi-Fi
    wifi_manager.wifi_init();
    wifi_manager.set_got_ip_callback(wifi_got_ip_event_callback);

    // Initialize Webserver
    ESP_LOGI(TAG, "Starting config web server");
    webserver.start_webserver();
    webserver.set_wifi_credentials_received_event_callback(&wifi_credentials_received_callback);
    webserver.set_ssid_to_show_on_config_page(wifi_manager.get_sta_ssid());

    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // Initialize MQTT
    ESP_LOGI(TAG, "Starting MQTT client");
    mqtt_app_start();
    vTaskDelay(10000 / portTICK_PERIOD_MS);

    /* Main loop */
    while (true)
    {
        publish_to_mqtt();
        vTaskDelay(120000 / portTICK_PERIOD_MS);
    }
}