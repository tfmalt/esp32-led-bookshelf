#ifndef SERIALMQTT_H
#define SERIALMQTT_H
#ifdef IS_TEENSY

#include <Arduino.h>

#include <functional>

typedef std::function<void(std::string, std::string)> CallbackOnMessage;
typedef std::function<void()> CallbackOnReady;
typedef std::function<void(std::string)> CallbackOnError;

#define BAUD_RATE 962100

class SerialMQTT {
 public:
  SerialMQTT() {}

  void setup() {
    // Setting up serial connection
    Serial3.begin(BAUD_RATE, SERIAL_8N1);
  }

  void handle() {
    if (Serial3.available() > 0) {
      readData();
    }
  }

  void loop() { handle(); }

  SerialMQTT &onMessage(CallbackOnMessage _c) {
    _onMessage = _c;
    return *this;
  }

  SerialMQTT &onReady(CallbackOnReady _c) {
    _onReady = _c;
    return *this;
  }

  SerialMQTT &onDisconnect(CallbackOnError _c) {
    _onDisconnect = _c;
    return *this;
  }

  SerialMQTT &onError(CallbackOnError _c) {
    _onError = _c;
    return *this;
  }

  bool publish(std::string topic, std::string message) {
    std::string head = "TOPIC " + topic;
    std::string body = "MESSAGE " + message;
    Serial3.println(head.c_str());
    Serial3.println(body.c_str());
    Serial3.println();

    return true;
  }

  void publishInformationData() { Serial.println("DATA"); }

 private:
  CallbackOnMessage _onMessage;
  CallbackOnReady _onReady;
  CallbackOnError _onDisconnect;
  CallbackOnError _onError;

  void readData() {
    char c;
    char end = '\n';

    while (Serial3.available() > 0) {
      c = Serial3.read();
    }
  }
};

#endif  // IS_TEENSY
#endif  // SERIALMQTT_H