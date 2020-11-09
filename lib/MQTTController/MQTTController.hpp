
#ifndef MQTTController_h
#define MQTTController_h

#ifdef IS_ESP32

#include <Arduino.h>
#include <ArduinoOTA.h>
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
  WiFiController wifiCtrl;

  MQTTController(){};

  MQTTController &setup();
  MQTTController &onReady(std::function<void()> callback);
  MQTTController &onDisconnect(std::function<void(std::string msg)> callback);
  MQTTController &onError(std::function<void(std::string error)> callback);
  MQTTController &onMessage(
      std::function<void(std::string, std::string)> callback);

  void checkConnection();
  bool publish(const char *topic, const char *message);
  bool publish(std::string topic, std::string message);
  // void publishInformation(const char *message);
  void publishInformationData();

 private:
  std::function<void(std::string, std::string)> _onMessage;
  std::function<void()> _onReady;
  std::function<void(std::string)> _onDisconnect;
  std::function<void(std::string)> _onError;

  void connect();
};
#endif  // IS_ESP32
#endif  // MQTTController_h