#ifndef SERIALMQTT_H
#define SERIALMQTT_H
#ifdef TEENSY

#include <Arduino.h>
#include <SerialTransfer.h>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

typedef std::function<void(std::string, std::string)> CallbackOnMessage;
typedef std::function<void()> CallbackOnReady;
typedef std::function<void(std::string)> CallbackOnDisconnect;
typedef std::function<void(std::string)> CallbackOnError;

constexpr uint32_t HEARTBEAT_MAX_AGE = 22000;

enum MQTTMessageType {
  CONNECT,
  CONNECT_ACK,
  SUBSCRIBE,
  UNSUBSCRIBE,
  STATUS_NO_SUB,
  STATUS_OK,
  PUBLISH,
  MESSAGE
};

enum MQTTQoS { AT_MOST_ONCE = 0, AT_LEAST_ONCE = 1, EXACTLY_ONCE = 2 };

struct MQTTMessage {
  uint8_t type;
  uint8_t count;
  uint8_t qos;
  char topic[32];
  char message[256];
};

// extern bool VERBOSE = false;

class SerialMQTT {
 public:
  SerialMQTT();

  void setup();
  void loop();
  bool connected();
  uint32_t getHeartbeatAge();

  SerialMQTT& connect();
  SerialMQTT& onMessage(CallbackOnMessage _c);
  SerialMQTT& onReady(CallbackOnReady _c);
  SerialMQTT& onMissingSubscribe(CallbackOnReady _c);
  SerialMQTT& onDisconnect(CallbackOnError _c);
  SerialMQTT& onError(CallbackOnError _c);

  SerialMQTT& enableVerboseOutput(bool v = true);
  SerialMQTT& subscribe(std::string topic);

  bool publish(std::string topic, std::string message);
  void publishInformationData();

 private:
  std::vector<CallbackOnReady> _onReadyList;
  std::vector<CallbackOnMessage> _onMessageList;
  std::vector<CallbackOnDisconnect> _onDisconnectList;
  std::vector<CallbackOnError> _onErrorList;
  std::vector<CallbackOnReady> _onMissingSubscribeList;

  SerialTransfer rxtx;
  bool VERBOSE = false;
  bool _isConnected = false;
  uint32_t _lastHeartbeat = millis();

  void checkConnection();
  void handleConnection();

  void emitMissingSubscribe();
  void emitReady();
  void emitDisconnect(std::string msg);
  void emitMessage(std::string topic, std::string msg);
};

#endif  // TEENSY
#endif  // SERIALMQTT_H