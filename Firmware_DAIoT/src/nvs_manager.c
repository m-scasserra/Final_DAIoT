#include "nvs_manager.h"
#include "esp_log.h"

esp_err_t start_nvs()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    return ret;
}

esp_err_t save_wifi_credentials(const char *ssid, const char *password)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(WIFI_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK)
    {
        return err;
    }

    err = nvs_set_str(nvs_handle, "ssid", ssid);
    if (err == ESP_OK)
    {
        err = nvs_set_str(nvs_handle, "password", password);
    }

    if (err == ESP_OK)
    {
        err = nvs_commit(nvs_handle);
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t load_wifi_credentials(char *ssid, char *password, size_t ssid_size, size_t pass_size)
{
    nvs_handle_t nvs_handle;
    size_t required_size;
    esp_err_t err = nvs_open(WIFI_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err != ESP_OK)
    {
        return err;
    }

    required_size = ssid_size;
    err = nvs_get_str(nvs_handle, "ssid", ssid, &required_size);
    
    if (err != ESP_OK)
    {
        nvs_close(nvs_handle);
        return err;
    }

    required_size = pass_size;
    err = nvs_get_str(nvs_handle, "password", password, &required_size);
    nvs_close(nvs_handle);

    return err;
}

#define PARAM_NAMESPACE "ESP32_WFM"
static const char *TAG = "wifi module miscs";

esp_err_t get_config_param_str(char *name, char **param)
{
    nvs_handle_t nvs;

    esp_err_t err = nvs_open(PARAM_NAMESPACE, NVS_READONLY, &nvs);
    if (err == ESP_OK)
    {
        size_t len;
        if ((err = nvs_get_str(nvs, name, NULL, &len)) == ESP_OK)
        {
            *param = (char *)malloc(len);
            err = nvs_get_str(nvs, name, *param, &len);
            ESP_LOGI(TAG, "Param readed: %s/%s", name, *param);
        }
        else
        {
            return err;
        }
        nvs_close(nvs);
    }
    else
    {
        return err;
    }
    return ESP_OK;
}

/* 'set_sta' command */
esp_err_t set_config_param_str(char *name, char *param)
{
    esp_err_t err;
    nvs_handle_t nvs;

    ESP_LOGI(TAG, "Opening NVS_READWRITE");
    err = nvs_open(PARAM_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK)
    {
        return err;
    }

    ESP_LOGI(TAG, "NVS Writing param...");
    err = nvs_set_str(nvs, name, param);
    if (err == ESP_OK)
    {
        err = nvs_commit(nvs);
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "Param writed: %s/%s", name, param);
        }
    }

    ESP_LOGI(TAG, "Closing NVS_READWRITE");
    nvs_close(nvs);
    return err;
}