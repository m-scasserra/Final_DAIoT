#ifndef NVS_MANAGER_H
#define NVS_MANAGER_H

#include "esp_err.h"
#include "nvs_flash.h"

#define WIFI_NAMESPACE "wifi"

esp_err_t start_nvs();
esp_err_t save_wifi_credentials(const char *ssid, const char *password);
esp_err_t load_wifi_credentials(char *ssid, char *password, size_t ssid_size, size_t pass_size);
esp_err_t get_config_param_str(char *name, char **param);
esp_err_t set_config_param_str(char *name, char *param);

#endif /* NVS_MANAGER_H */