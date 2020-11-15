#ifdef TEENSY
#include "SerialMQTT.hpp"
#include <Arduino.h>
#include <I2CTransfer.h>

I2CTransfer rxtx;

void handleMessage() {
  MQTTMessage incoming;
  rxtx.rxObj(incoming);
  Serial.println("[mqtt] got i2c callback:");
  Serial.printf("[mqtt] topic: %s, message: %s", incoming.topic.c_str(),
                incoming.message.c_str());
}

functionPtr rxtxCallbacks[] = {handleMessage};

SerialMQTT::SerialMQTT(){};

void SerialMQTT::setup() {
  // Setting up serial connection
  Serial.println("[mqtt] running setup for SerialMQTT");

  Wire.begin(0);

  configST conf;
  conf.debug = true;
  conf.callbacks = rxtxCallbacks;
  conf.callbacksLen = sizeof(rxtxCallbacks) / sizeof(functionPtr);

  Serial.printf("[mqtt] callbackslen: %i\n", conf.callbacksLen);

  rxtx.begin(Wire, conf);
}

void SerialMQTT::loop() {
  // hello
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
  // std::string head = "TOPIC " + topic;
  // std::string body = "MESSAGE " + message;
  // Serial3.println(head.c_str());
  // Serial3.println(body.c_str());
  // Serial3.println();

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