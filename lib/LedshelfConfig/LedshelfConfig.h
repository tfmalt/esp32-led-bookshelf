/**
 * Class for reading and accessing configuration
 */
#ifndef LedshelfConfig_h
#define LedshelfConfig_h
#include <Arduino.h>

class LedshelfConfig {
    public:
        LedshelfConfig();
        void setup();
        const char* ssid()          const { return wifi_ssid.c_str(); };
        const char* psk()           const { return wifi_psk.c_str(); };
        const char* state_topic()   const { return mqtt_state_topic.c_str(); };
        const char* command_topic() const { return mqtt_command_topic.c_str(); };
        const char* status_topic()  const { return mqtt_status_topic.c_str(); };
        const char* server()        const { return mqtt_server.c_str(); };
        uint16_t    port()          const { return mqtt_port; };
        const char* username()      const { return mqtt_username.c_str(); };
        const char* password()      const { return mqtt_password.c_str(); };
        const char* client()        const { return mqtt_client.c_str(); };
        const char* ca_root()       const { return ca_root_data.c_str(); };

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
