#ifndef SerialMQTTTransfer_H
#define SerialMQTTTransfer_H
#ifdef TEENSY

#include <AbstractMQTTController.h>
#include <Arduino.h>
#include <SerialTransfer.h>
#include <algorithm>
#include <functional>
#include <string>
#include <vector>

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

class SerialMQTTTransfer : public AbstractMQTTController {
 public:
  SerialMQTTTransfer();

  void begin();
  void loop();
  bool connect();
  bool connected();

  bool publish(std::string topic, std::string message);
  bool publish(const char* topic, const char* message);
  bool subscribe(std::string topic);
  bool subscribe(const char* topic);

  uint32_t getHeartbeatAge();

  SerialMQTTTransfer& onMissingSubscribe(OnReadyFunction _c);

  void publishInformationData();

 private:
  std::vector<OnReadyFunction> _onMissingSubscribeList;

  SerialTransfer rxtx;

  bool _isConnected = false;
  uint32_t _lastHeartbeat = millis();

  void checkConnection();
  void handleConnection();

  void emitMissingSubscribe();
};

#endif  // TEENSY
#endif  // SerialMQTTTransfer_H