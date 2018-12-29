/**
 * Class for reading and accessing configuration
 */
#ifndef LedshelfConfig_h
#define LedshelfConfig_h
#include <Arduino.h>
#include <vector>

typedef struct LightConfig {
    String name;
    uint16_t top[2];
    uint16_t bottom[2];
    String command_topic;
    String state_topic;
} LightConfig;

// typedef struct Lights {
//
// };

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
            query_topic(mqtt_query_topic),
            information_topic(mqtt_information_topic),
            update_topic(mqtt_update_topic),
            port(mqtt_port),
            username(mqtt_username),
            password(mqtt_password),
            client(mqtt_client),
            num_leds(mqtt_num_leds),
            milliamps(mqtt_milliamps)
        {};

        void setup();

        const String   &ca_root;
        const String   &ssid;
        const String   &psk;
        const String   &server;
        const String   &state_topic;
        const String   &command_topic;
        const String   &status_topic;
        const String   &query_topic;
        const String   &information_topic;
        const String   &update_topic;
        const uint16_t &port;
        const String   &username;
        const String   &password;
        const String   &client;
        const uint16_t &num_leds;
        const uint16_t &milliamps;

        std::vector<LightConfig> lights;

        String stateTopic();
        String commandTopic();
        String statusTopic();
        String queryTopic();
        String informationTopic();
        String updateTopic();

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
        String      mqtt_query_topic;
        String      mqtt_information_topic;
        String      mqtt_update_topic;
        uint16_t    mqtt_num_leds;
        uint16_t    mqtt_milliamps;

        void parseConfigFile();
        void readCAFile();
};

#endif // LedshelfConfig_h
