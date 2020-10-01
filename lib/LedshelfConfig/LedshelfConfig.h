/**
 * Class for reading and accessing configuration
 */
#ifndef LEDSHELFCONFIG_H
#define LEDSHELFCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

class LedshelfConfig {
 public:
  // char ca_root[1680];
  char wifi_ssid[64];
  char wifi_psk[64];
  char wifi_hostname[16];
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

  LedshelfConfig(){};

  void setup() {
    if (!SPIFFS.begin()) {
      ESP.restart();
      return;
    }

    parseConfigFile();
  }

  void stateTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_state_topic);
  }

  void commandTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_command_topic);
  }

  void statusTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_status_topic);
  }

  void queryTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_query_topic);
  }

  void informationTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_information_topic);
  }

  void updateTopic(char *topic) {
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_update_topic);
  }

 private:
  void parseConfigFile() {
#ifdef DEBUG
    Serial.printf("  - Parsing config file: %s ... ", CONFIG_FILE);
#endif

    File file = SPIFFS.open(CONFIG_FILE, "r");

    if (!file) return;

    StaticJsonDocument<680> config;
    auto error = deserializeJson(config, file);

    file.close();

    if (error) return;

    strlcpy(wifi_ssid, config["wifi"]["ssid"] | "", sizeof(wifi_ssid));
    strlcpy(wifi_psk, config["wifi"]["psk"] | "", sizeof(wifi_psk));
    strlcpy(wifi_hostname, config["wifi"]["hostname"] | "ledshelf",
            sizeof(wifi_hostname));
    strlcpy(mqtt_server, config["mqtt"]["server"] | "", sizeof(mqtt_server));
    mqtt_port = config["mqtt"]["port"] | 1883;
    mqtt_ssl = config["mqtt"]["ssl"] | false;
    strlcpy(mqtt_username, config["mqtt"]["username"] | "",
            sizeof(mqtt_username));
    strlcpy(mqtt_password, config["mqtt"]["password"] | "",
            sizeof(mqtt_password));
    strlcpy(mqtt_client, config["mqtt"]["client"] | "", sizeof(mqtt_client));
    strlcpy(mqtt_command_topic, config["mqtt"]["topics"]["command"] | "",
            sizeof(mqtt_command_topic));
    strlcpy(mqtt_state_topic, config["mqtt"]["topics"]["state"] | "",
            sizeof(mqtt_state_topic));
    strlcpy(mqtt_status_topic, config["mqtt"]["topics"]["status"] | "",
            sizeof(mqtt_status_topic));
    strlcpy(mqtt_query_topic, config["mqtt"]["topics"]["query"] | "",
            sizeof(mqtt_query_topic));
    strlcpy(mqtt_information_topic,
            config["mqtt"]["topics"]["information"] | "",
            sizeof(mqtt_information_topic));
    strlcpy(mqtt_update_topic, config["mqtt"]["topics"]["update"] | "",
            sizeof(mqtt_update_topic));

#ifdef DEBUG
    Serial.println("Done");
#endif
  }

  /**
   * Reads the TLS CA Root Certificate from file.
   */
  // void readCAFile() {
  //   File file = SPIFFS.open("/ca.pem", "r");
  //
  //     if (!file) return;
  //
  //     strcpy(ca_root, "");
  //     while (file.available()) {
  //       strcat(ca_root, file.readString().c_str());
  //     }
  //
  //     file.close();
  //   }
};

#endif  // LEDSHELFCONFIG_H
