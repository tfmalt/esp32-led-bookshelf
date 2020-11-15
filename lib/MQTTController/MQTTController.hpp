
#ifndef MQTTController_h
#define MQTTController_h
#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include <functional>
#include <string>

typedef std::function<void()> OnReadyFunction;
typedef std::function<void(std::string)> OnDisconnectFunction;
typedef std::function<void(std::string)> OnErrorFunction;
typedef std::function<void(std::string, std::string)> OnMessageFunction;

class MQTTController {
 public:
  const char* version = VERSION;

  LedshelfConfig config;
  PubSubClient client;
  WiFiController wifiCtrl;

  MQTTController();
  ~MQTTController(){};

  MQTTController& setup();
  MQTTController& onReady(OnReadyFunction callback);
  MQTTController &onDisconnect(OnDisconnectFunction msg)> callback);
  MQTTController &onError(OnErrorFunction error)> callback);
  MQTTController& onMessage(OnMessageFunction callback);

  void loop();
  bool publish(const char* topic, const char* message);
  bool publish(std::string topic, std::string message);
  // void publishInformation(const char *message);
  void publishInformationData();

 private:
  OnMessageFunction _onMessage;
  OnReadyFunction _onReady;
  OnDisconnectFunction _onDisconnect;
  OnErrorFunction _onError;

  void connect();
};
#endif  // MQTTController_h