/**
 * Class for reading and accessing configuration
 */
#ifndef LedshelfConfig_h
#define LedshelfConfig_h
#include <Arduino.h>

class LedshelfConfig
{
public:
    LedshelfConfig(){};

    void setup();

    char ca_root[1680];
    char wifi_ssid[64];
    char wifi_psk[64];
    char mqtt_server[32];
    uint16_t mqtt_port;
    char mqtt_username[32];
    char mqtt_password[64];
    bool mqtt_ssl;
    char mqtt_client[32];
    char mqtt_command_topic[16];
    char mqtt_state_topic[16];
    char mqtt_status_topic[16];
    char mqtt_query_topic[16];
    char mqtt_information_topic[16];
    char mqtt_update_topic[16];
    uint16_t fastled_num_leds;
    uint16_t fastled_milliamps;

    void stateTopic(char *topic);
    void commandTopic(char *topic);
    void statusTopic(char *topic);
    void queryTopic(char *topic);
    void informationTopic(char *topic);
    void updateTopic(char *topic);

private:
    const char *configFile = "/config.json";
    const char *caFile = "/ca.pem";

    void parseConfigFile();
    void readCAFile();
};

#endif // LedshelfConfig_h
