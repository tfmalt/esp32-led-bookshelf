/**
 * Class for reading and accessing configuration
 */
#ifndef LEDSHELFCONFIG_H
#define LEDSHELFCONFIG_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Credentials.h>

#include <string>

class LedshelfConfig {
 public:
  std::string wifi_ssid = WIFI_SSID;
  std::string wifi_psk = WIFI_PSK;
  std::string wifi_hostname = WIFI_HOSTNAME;

  std::string mqtt_server = MQTT_SERVER;
  uint16_t mqtt_port = MQTT_PORT;

  std::string mqtt_username = MQTT_USER;
  std::string mqtt_password = MQTT_PASS;

  std::string mqtt_client = MQTT_CLIENT;
  std::string mqtt_command_topic = MQTT_TOPIC_COMMAND;
  std::string mqtt_state_topic = MQTT_TOPIC_STATE;
  std::string mqtt_status_topic = MQTT_TOPIC_STATUS;
  std::string mqtt_query_topic = MQTT_TOPIC_QUERY;
  std::string mqtt_information_topic = MQTT_TOPIC_INFORMATION;
  std::string mqtt_update_topic = MQTT_TOPIC_UPDATE;

  LedshelfConfig(){};

  void setup() {}

  void stateTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_state_topic;
    strcpy(topic, newtopic.c_str());
  }

  void commandTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_command_topic;
    strcpy(topic, newtopic.c_str());
  }

  void statusTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_status_topic;
    strcpy(topic, newtopic.c_str());
  }

  void queryTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_query_topic;
    strcpy(topic, newtopic.c_str());
  }

  void informationTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_information_topic;
    strcpy(topic, newtopic.c_str());
  }

  void updateTopic(char *topic) {
    std::string newtopic = "/" + mqtt_username + mqtt_update_topic;
    strcpy(topic, newtopic.c_str());
  }
};

#endif  // LEDSHELFCONFIG_H
