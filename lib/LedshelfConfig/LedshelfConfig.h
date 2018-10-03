/**
 * Class for reading and accessing configuration
 */
#ifndef LedshelfConfig_h
#define LedshelfConfig_h
#include <Arduino.h>

class LedshelfConfig {
    public:
        LedshelfConfig() :
            ca_root(ca_root_data),
            ssid(wifi_ssid),
            psk(wifi_psk),
            server(mqtt_server),
            state_topic(mqtt_state_topic),
            command_topic(mqtt_command_topic),
            status_topic(mqtt_status_topic),
            port(mqtt_port),
            username(mqtt_username),
            password(mqtt_password),
            client(mqtt_client)
        {};

        void setup();

        const String   &ca_root;
        const String   &ssid;
        const String   &psk;
        const String   &server;
        const String   &state_topic;
        const String   &command_topic;
        const String   &status_topic;
        const uint16_t &port;
        const String   &username;
        const String   &password;
        const String   &client;

    private:
        const String configFile = "/config.json";
        const String caFile     = "/ca.pem";

        String      ca_root_data;
        String      wifi_ssid;
        String      wifi_psk;
        String      mqtt_server;
        uint16_t    mqtt_port;
        String      mqtt_username;
        String      mqtt_password;
        String      mqtt_client;
        String      mqtt_command_topic;
        String      mqtt_state_topic;
        String      mqtt_status_topic;

        void parseConfigFile();
        void readCAFile();
};

#endif // LedshelfConfig_h
