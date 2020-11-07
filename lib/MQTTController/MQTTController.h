
#ifndef MQTTController_h
#define MQTTController_h

#ifdef IS_ESP32

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Effects.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiController.h>

class MQTTController {
 public:
  const char *version;
  LedshelfConfig &config;
  PubSubClient client;
  WiFiController &wifiCtrl;
  LightStateController &lightState;
  Effects &effects;

  char statusTopic[32];
  char commandTopic[32];
  char updateTopic[32];
  char stateTopic[32];
  char informationTopic[32];
  char queryTopic[32];

  // MQTTController();
  MQTTController(const char *v_, LedshelfConfig &c_, WiFiController &w_,
                 LightStateController &l_, Effects &e_)
      : version(v_), config(c_), wifiCtrl(w_), lightState(l_), effects(e_){};

  void setup();
  void checkConnection();
  void publishInformation(const char *message);
  void publishInformationData();
  void publishStatus();

 private:
  void publishState();
  void handleNewState(LightState &state);
  void handleUpdate();
  void callback(char *p_topic, byte *p_message, unsigned int p_length);
  void connect();
};
#endif  // IS_ESP32
#endif  // MQTTController_h