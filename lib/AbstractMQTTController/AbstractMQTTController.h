
#ifndef ABSTRACT_MQTT_CONTROLLER_H
#define ABSTRACT_MQTT_CONTROLLER_H
#include <Arduino.h>
#include <functional>
#include <string>
#include <vector>

typedef std::function<void()> OnReadyFunction;
typedef std::function<void(std::string)> OnDisconnectFunction;
typedef std::function<void(std::string)> OnErrorFunction;
typedef std::function<void(std::string, std::string)> OnMessageFunction;

class AbstractMQTTController {
 public:
  const char* version = VERSION;

  virtual void begin() = 0;
  virtual void loop() = 0;
  virtual bool connect() = 0;
  virtual bool connected() = 0;

  AbstractMQTTController& onReady(OnReadyFunction callback);
  AbstractMQTTController& onDisconnect(OnDisconnectFunction msg);
  AbstractMQTTController& onError(OnErrorFunction error);
  AbstractMQTTController& onMessage(OnMessageFunction callback);
  AbstractMQTTController& emitReady();
  AbstractMQTTController& emitDisconnect(std::string);
  AbstractMQTTController& emitError(std::string);
  AbstractMQTTController& emitMessage(std::string, std::string);

  AbstractMQTTController& enableVerboseOutput() {
    return enableVerboseOutput(true);
  };

  AbstractMQTTController& enableVerboseOutput(bool v);
  bool verbose() { return _verbose; };

  virtual bool publish(const char* topic, const char* message) = 0;
  virtual bool publish(std::string topic, std::string message) = 0;
  virtual bool subscribe(std::string topic) = 0;
  virtual bool subscribe(const char* topic) = 0;
  // void publishInformation(const char *message);
  // void publishInformationData();

 private:
  OnMessageFunction _onMessage;
  OnReadyFunction _onReady;
  OnDisconnectFunction _onDisconnect;
  OnErrorFunction _onError;

  std::vector<std::string> subscriptions;

  bool _verbose = false;
};
#endif  // ABSTRACT_MQTT_CONTROLLER_H