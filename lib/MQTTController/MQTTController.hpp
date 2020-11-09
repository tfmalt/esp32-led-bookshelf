
#ifndef MQTTController_h
#define MQTTController_h

#ifdef IS_ESP32

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Effects.h>
#include <LightStateController.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <LedshelfConfig.hpp>
#include <WiFiController.hpp>
#include <functional>
#include <string>

class MQTTController {
 public:
  const char *version = VERSION;
  LedshelfConfig config;
  PubSubClient client;

  LightStateController *lightState;
  Effects *effects;

  WiFiController wifiCtrl;

  char statusTopic[32];
  char commandTopic[32];
  char updateTopic[32];
  char stateTopic[32];
  char informationTopic[32];
  char queryTopic[32];

  // MQTTController();
  MQTTController(){};
  //  , LightStateController &l_,
  //                 Effects &e_)
  //      : version(v_), config(c_), lightState(l_), effects(e_),
  //      wifiCtrl(c_){};

  MQTTController &setup();
  MQTTController &onReady(std::function<void()> callback);
  MQTTController &onDisconnect(std::function<void(std::string msg)> callback);
  MQTTController &onError(std::function<void(std::string error)> callback);
  MQTTController &onMessage(
      std::function<void(std::string, std::string)> callback);

  void checkConnection();
  void publishInformation(const char *message);
  void publishInformationData();
  void publishStatus();

 private:
  std::function<void(std::string, std::string)> _onMessage;
  std::function<void()> _onReady;
  std::function<void(std::string)> _onDisconnect;
  std::function<void(std::string)> _onError;

  void publishState();
  void handleNewState(LightState &state);
  void handleUpdate();
  void callback_f(char *p_topic, byte *p_message, unsigned int p_length);
  void connect();
};
#endif  // IS_ESP32
#endif  // MQTTController_h