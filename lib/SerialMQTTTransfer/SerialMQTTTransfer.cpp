#ifdef TEENSY
#include "SerialMQTTTransfer.hpp"
#include <Arduino.h>

#define SERIAL_BAUD_RATE 57600
uint32_t start_time = 0;

SerialMQTTTransfer::SerialMQTTTransfer(){};

void SerialMQTTTransfer::begin() {
  Serial.println("[mqtt] running setup for SerialMQTTTransfer.");
  Serial3.begin(RXTX_BAUD_RATE, SERIAL_8N1);
  rxtx.begin(Serial3);
}

SerialMQTTTransfer& SerialMQTTTransfer::connect() {
  MQTTMessage msg;

  if (millis() - start_time > 1000) {
    Serial.printf("[mqtt] ||| serial disconnected: %lu\n", getHeartbeatAge());
    msg.type = MQTTMessageType::CONNECT;
    strcpy(msg.topic, "CONNECT");
    strcpy(msg.message, "");

    rxtx.sendDatum(msg);
    start_time = millis();
  }

  return *this;
}

void SerialMQTTTransfer::loop() {
  if (rxtx.available()) {
    MQTTMessage msg;
    // msg.count = 0;
    rxtx.rxObj(msg);
    this->_lastHeartbeat = millis();
    switch (msg.type) {
      case MQTTMessageType::MESSAGE: {
        Serial.printf("[mqtt] received topic: %s, message: %s\n", msg.topic,
                      msg.message);

        this->emitMessage(std::string{msg.topic}, std::string{msg.message});
        break;
      }

      case MQTTMessageType::CONNECT_ACK: {
        this->handleConnection();
        break;
      }

      case MQTTMessageType::STATUS_OK: {
        Serial.printf("[mqtt] <<< STATUS OK: count: %i\n", msg.count);
        // this->_isConnected = true;
        if (msg.count < 2) {
          this->emitMissingSubscribe();
        }
        break;
      }

      case MQTTMessageType::STATUS_NO_SUB: {
        Serial.printf("[mqtt] <<< NO SUBSCRIPTIONS: %i\n", msg.count);
        this->emitMissingSubscribe();
        break;
      }

      default: {
        Serial.printf("[mqtt] <<< unknown message: type: %i, topic: %s\n",
                      msg.type, msg.topic);
      }
    }
  }

  checkConnection();
}

// ==========================================================================
// event handler callback registration
// ==========================================================================
SerialMQTTTransfer& SerialMQTTTransfer::onMessage(CallbackOnMessage _c) {
  this->_onMessageList.push_back(_c);
  return *this;
}

SerialMQTTTransfer& SerialMQTTTransfer::onReady(CallbackOnReady _c) {
  this->_onReadyList.push_back(_c);
  return *this;
}
SerialMQTTTransfer& SerialMQTTTransfer::onMissingSubscribe(CallbackOnReady _c) {
  this->_onMissingSubscribeList.push_back(_c);
  return *this;
}

SerialMQTTTransfer& SerialMQTTTransfer::onDisconnect(CallbackOnDisconnect _c) {
  this->_onDisconnectList.push_back(_c);
  return *this;
}

SerialMQTTTransfer& SerialMQTTTransfer::onError(CallbackOnError _c) {
  this->_onErrorList.push_back(_c);
  return *this;
}

// ==========================================================================
// event emitters
// ==========================================================================
void SerialMQTTTransfer::emitMissingSubscribe() {
  for (auto func : this->_onMissingSubscribeList) {
    if (func != nullptr) {
      func();
    }
  }
}

void SerialMQTTTransfer::emitReady() {
  for (auto func : this->_onReadyList) {
    if (func != nullptr) {
      func();
    }
  }
}

void SerialMQTTTransfer::emitMessage(std::string topic, std::string msg) {
  for (auto func : this->_onMessageList) {
    if (func != nullptr) {
      func(topic, msg);
    }
  }
}

// ==========================================================================
// MQTT passthrough functions.
// ==========================================================================
SerialMQTTTransfer& SerialMQTTTransfer::subscribe(std::string topic) {
  MQTTMessage msg;
  msg.type = MQTTMessageType::SUBSCRIBE;
  strcpy(msg.topic, topic.c_str());
  strcpy(msg.message, "");

  rxtx.sendDatum(msg);

  return *this;
}

bool SerialMQTTTransfer::publish(std::string topic, std::string message) {
  MQTTMessage msg;
  msg.type = MQTTMessageType::MESSAGE;
  strcpy(msg.topic, topic.c_str());
  strcpy(msg.message, message.c_str());
  rxtx.sendDatum(msg);

  return true;
}

void SerialMQTTTransfer::publishInformationData() {
  Serial.println("DATA");
}

SerialMQTTTransfer& SerialMQTTTransfer::enableVerboseOutput(bool v) {
  VERBOSE = v;
  return *this;
}

uint32_t SerialMQTTTransfer::getHeartbeatAge() {
  return (millis() - this->_lastHeartbeat);
}

bool SerialMQTTTransfer::connected() {
  return _isConnected;
}

void SerialMQTTTransfer::checkConnection() {
  if (this->getHeartbeatAge() > HEARTBEAT_MAX_AGE) {
    _isConnected = false;

    // for (auto func : this->_onDisconnectList) {
    //   if (func != nullptr) {
    //     func(std::string{"Did not get a heartbeat in time."});
    //   }
    // }
  }

  if (!this->connected()) {
    this->connect();
  }
}

void SerialMQTTTransfer::handleConnection() {
  this->_isConnected = true;
  this->_lastHeartbeat = millis();

  Serial.println("[mqtt] ||| serial connected.");

  this->emitReady();
}

#endif