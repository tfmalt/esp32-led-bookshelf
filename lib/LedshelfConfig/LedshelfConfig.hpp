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
#ifdef IS_ESP32
  std::string wifi_ssid = WIFI_SSID;
  std::string wifi_psk = WIFI_PSK;
  std::string wifi_hostname = WIFI_HOSTNAME;

  std::string mqtt_server = MQTT_SERVER;
  uint16_t mqtt_port = MQTT_PORT;

  std::string mqtt_password = MQTT_PASS;
  std::string mqtt_client = MQTT_CLIENT;
#endif  // IS_ESP32

  std::string mqtt_username = MQTT_USER;

  std::string mqtt_command_topic = MQTT_TOPIC_COMMAND;
  std::string mqtt_state_topic = MQTT_TOPIC_STATE;
  std::string mqtt_status_topic = MQTT_TOPIC_STATUS;
  std::string mqtt_query_topic = MQTT_TOPIC_QUERY;
  std::string mqtt_information_topic = MQTT_TOPIC_INFORMATION;
  std::string mqtt_update_topic = MQTT_TOPIC_UPDATE;

  std::string state_topic;
  std::string command_topic;
  std::string status_topic;
  std::string query_topic;
  std::string information_topic;
  std::string update_topic;

  LedshelfConfig() {
    state_topic = "/" + mqtt_username + mqtt_state_topic;
    command_topic = "/" + mqtt_username + mqtt_command_topic;
    status_topic = "/" + mqtt_username + mqtt_status_topic;
    query_topic = "/" + mqtt_username + mqtt_query_topic;
    information_topic = "/" + mqtt_username + mqtt_information_topic;
    update_topic = "/" + mqtt_username + mqtt_update_topic;
  };

  void setup() {}

  std::string &stateTopic() { return state_topic; }
  void stateTopic(char *topic) { strcpy(topic, state_topic.c_str()); }

  std::string &commandTopic() { return command_topic; }
  void commandTopic(char *topic) { strcpy(topic, command_topic.c_str()); }

  std::string &statusTopic() { return status_topic; }
  void statusTopic(char *topic) { strcpy(topic, status_topic.c_str()); }

  std::string &queryTopic() { return query_topic; }
  void queryTopic(char *topic) { strcpy(topic, query_topic.c_str()); }

  std::string &informationTopic() { return information_topic; }
  void informationTopic(char *topic) {
    strcpy(topic, information_topic.c_str());
  }

  std::string &updateTopic() { return update_topic; };
  void updateTopic(char *topic) { strcpy(topic, update_topic.c_str()); }
};

#endif  // LEDSHELFCONFIG_H
