#ifndef HTTP_SERVER_
#define HTTP_SERVER_

#include <esp_http_server.h>

typedef struct
{
    char ssid[64];
    char pass[64];
} wifi_credentials_t;

typedef struct
{
    char esp_idf_version[64];
    char esp_cores[10];
    char wifi_sta_mac[30];
    char wifi_ap_mac[30];
    char wifi_bt_mac[30];
    char wifi_sta_ip[30];
} device_data_t;

/************************************************************************/
/* La siguiente estructura simula un objeto en C                        */
/*                                                                      */
/* Las propiedades estan implementadas comp punteros a variables del .c */
/* Los metodos son punteros a funciones definidas dentro del .c         */
/************************************************************************/
typedef struct
{
    // WebServer Props
    wifi_credentials_t *wifi_credentials;
    // WebServer Functions
    void (*start_webserver)(void);
    void (*stop_webserver)(void);
    void (*set_wifi_credentials_received_event_callback)(void *callback);
    void (*set_ssid_to_show_on_config_page)(char *ssid);
    void (*set_sta_ip_to_show_on_config_page)(char *ssid);

} webserver_t;

/************************************************************************/
/* La declaración de esta variable se realiza dentro del .c             */
/*                                                                      */
/* Agregar este "extern" en el archivo de cabecera, permite que         */
/* cualquier .c que lo incluya con #include, tenga acceso al "objeto".  */
/************************************************************************/
extern const webserver_t webserver;

#endif /* HTTP_SERVER_ */