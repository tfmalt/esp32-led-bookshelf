# Stub for adding a Credentials.h file

To setup credentials for the wifi and mqtt broker correctly you need to create a file named Credentials.h in this folder

```C++
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#define WIFI_SSID ("<ssid>")
#define WIFI_PSK ("<wifi password>")

#define MQTT_SERVER ("<mqtt server>")
#define MQTT_PORT <mqtt port>

#define WIFI_HOSTNAME ("hostname")
#define MQTT_USER ("username")
#define MQTT_PASS ("mqtt password")
#define MQTT_CLIENT ("client name")

#define MQTT_TOPIC_STATUS ("/status")
#define MQTT_TOPIC_QUERY ("/query")
#define MQTT_TOPIC_UPDATE ("/update")
#define MQTT_TOPIC_COMMAND ("/light/set")
#define MQTT_TOPIC_STATE ("/light/state")
#define MQTT_TOPIC_INFORMATION ("/information")

#endif  // CREDENTIALS_H
```
