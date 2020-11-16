#ifdef TEENSY
#include "SerialMQTT.hpp"
#include <Arduino.h>
// #include <FastLED.h>
// #include <SerialTransfer.h>

#define RXTX_BAUD_RATE 57600

SerialMQTT::SerialMQTT(){};

void SerialMQTT::setup() {
  Serial.println("[mqtt] running setup for SerialMQTT");
  Serial3.begin(RXTX_BAUD_RATE, SERIAL_8N1);
  rxtx.begin(Serial3);
}

void SerialMQTT::loop() {
  // EVERY_N_SECONDS(2) { Serial.println("[mqtt] inside serial mqtt loop."); }
  MQTTMessage msg;

  if (rxtx.available()) {
    rxtx.rxObj(msg);
    Serial.printf("[mqtt] received topic: %s, message: %s\n", msg.topic,
                  msg.message);

    if (this->_onMessage != nullptr) {
      this->_onMessage(std::string{msg.topic}, std::string{msg.message});
    }
  }
}

SerialMQTT& SerialMQTT::onMessage(CallbackOnMessage _c) {
  _onMessage = _c;
  return *this;
}

SerialMQTT& SerialMQTT::onReady(CallbackOnReady _c) {
  _onReady = _c;
  return *this;
}

SerialMQTT& SerialMQTT::onDisconnect(CallbackOnDisconnect _c) {
  _onDisconnect = _c;
  return *this;
}

SerialMQTT& SerialMQTT::onError(CallbackOnError _c) {
  _onError = _c;
  return *this;
}

bool SerialMQTT::publish(std::string topic, std::string message) {
  MQTTMessage msg;
  strcpy(msg.topic, topic.c_str());
  strcpy(msg.message, message.c_str());
  rxtx.sendDatum(msg);

  return true;
}

void SerialMQTT::publishInformationData() {
  Serial.println("DATA");
}

SerialMQTT& SerialMQTT::enableVerboseOutput(bool v) {
  VERBOSE = v;
  return *this;
}

#endif