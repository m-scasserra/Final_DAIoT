#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

// Set your local broker URI
#define BROKER_URI "mqtts://bootdomain.duckdns.org:8883"

void mqtt_app_start(void);
void publish_to_mqtt(void);
#endif /* MQTT_MANAGER_H */