#ifndef SERIALMQTT_H
#define SERIALMQTT_H
#ifdef TEENSY

#include <Arduino.h>
#include <functional>
#include <string>

typedef std::function<void(std::string, std::string)> CallbackOnMessage;
typedef std::function<void()> CallbackOnReady;
typedef std::function<void(std::string)> CallbackOnDisconnect;
typedef std::function<void(std::string)> CallbackOnError;

struct MQTTMessage {
  std::string topic;
  std::string message;
};

// extern bool VERBOSE = false;

class SerialMQTT {
 public:
  SerialMQTT();

  void setup();
  void loop();

  SerialMQTT& onMessage(CallbackOnMessage _c);
  SerialMQTT& onReady(CallbackOnReady _c);
  SerialMQTT& onDisconnect(CallbackOnError _c);
  SerialMQTT& onError(CallbackOnError _c);

  SerialMQTT& enableVerboseOutput(bool v = true);

  bool publish(std::string topic, std::string message);
  void publishInformationData();

 private:
  CallbackOnMessage _onMessage;
  CallbackOnReady _onReady;
  CallbackOnError _onDisconnect;
  CallbackOnError _onError;

  bool VERBOSE = false;
};

#endif  // TEENSY
#endif  // SERIALMQTT_H