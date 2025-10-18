/* MQTT Mutual Authentication Example */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt_manager.h"
#include "led.h"

static const char *TAG = "MQTTS_EXAMPLE";
extern const uint8_t _binary_client_crt_start[];
extern const uint8_t _binary_client_crt_end[];
extern const uint8_t _binary_client_key_start[];
extern const uint8_t _binary_client_key_end[];
extern const uint8_t _binary_ca_crt_start[];
extern const uint8_t _binary_ca_crt_end[];

esp_mqtt_client_handle_t client = NULL;
uint8_t baseMac[6] = {0};
char topicData[350] = {0};
char topicCmd[350] = {0};
static bool luzEncendida = false;

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0)
    {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%ld", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_subscribe(client, topicCmd, 0);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        if (strncmp(event->topic, topicCmd, event->topic_len) == 0)
        {
            ESP_LOGI(TAG, "Comando recibido: %.*s", event->data_len, event->data);
            if (strncmp(event->data, "LUZ ON", strlen("LUZ ON")) == 0)
            {
                ESP_LOGI(TAG, "Encender la luz");
                set_led_color(255, 255, 255);
                show_led();
                luzEncendida = true;
            }
            else if (strncmp(event->data, "LUZ OFF", strlen("LUZ OFF")) == 0)
            {
                ESP_LOGI(TAG, "Apagar la luz");
                set_led_color(0, 0, 0);
                show_led();
                luzEncendida = false;
            }
            else if (strncmp(event->data, "REPORT", strlen("REPORT")) == 0)
            {
                ESP_LOGI(TAG, "Enviar reporte");
                publish_to_mqtt();
            }
            else
            {
                ESP_LOGI(TAG, "Comando no reconocido");
            }
        }

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno", event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .broker.verification.certificate = (const char *)_binary_ca_crt_start,
        .credentials.authentication.certificate = (const char *)_binary_client_crt_start,
        .credentials.authentication.key = (const char *)_binary_client_key_start};

    esp_err_t ret = ESP_OK;

    ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK)
    {
        snprintf(topicData, sizeof(topicData),
                 "/%02X%02X%02X%02X%02X%02X/data",
                 baseMac[0], baseMac[1], baseMac[2],
                 baseMac[3], baseMac[4], baseMac[5]);
        snprintf(topicCmd, sizeof(topicCmd),
                 "/%02X%02X%02X%02X%02X%02X/cmd",
                 baseMac[0], baseMac[1], baseMac[2],
                 baseMac[3], baseMac[4], baseMac[5]);
    }
    ESP_LOGI(TAG, "[APP] Free memory: %ld bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void publish_to_mqtt(void)
{
    ESP_LOGI(TAG, "Ingresa a publish_to_mqtt_topic()");

    char bufferJson[300] = {0};
    int msg_id = 0;
    char buffer_temp_txt[15] = {0};
    char buffer_humedad_txt[15] = {0};

    /*
    {
        "luz1":0,
        "luz2":0,
        "temperatura":16,
        "humedad":80
    }
    */
    strcat(bufferJson, "{\"luz1\":");
    strcat(bufferJson, luzEncendida ? "1" : "0");
    strcat(bufferJson, ",");
    strcat(bufferJson, "\"luz2\":1");
    strcat(bufferJson, ",");
    strcat(bufferJson, "\"temperatura\":");
    sprintf(buffer_temp_txt, "%d", rand() % 30 + 15);
    strcat(bufferJson, buffer_temp_txt);
    strcat(bufferJson, ",");
    strcat(bufferJson, "\"humedad\":");
    sprintf(buffer_humedad_txt, "%d", rand() % 30 + 15);
    strcat(bufferJson, buffer_humedad_txt);
    strcat(bufferJson, "}");

    ESP_LOGI(TAG, "JSON enviado:  %s", bufferJson);

    msg_id = esp_mqtt_client_publish(client, topicData, bufferJson, 0, 1, 0);

    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
}