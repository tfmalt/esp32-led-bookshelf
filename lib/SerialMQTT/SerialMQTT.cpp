#ifdef TEENSY
#include "SerialMQTT.hpp"
#include <Arduino.h>

#define RXTX_BAUD_RATE 57600
uint32_t start_time = 0;

SerialMQTT::SerialMQTT(){};

void SerialMQTT::setup() {
  Serial.println("[mqtt] running setup for SerialMQTT.");
  Serial3.begin(RXTX_BAUD_RATE, SERIAL_8N1);
  rxtx.begin(Serial3);

  // while (!this->connected()) {
  //   this->connect();
  // }
  //
  //   Serial.println();
  //   Serial.println("[mqtt] connected.");
}

SerialMQTT& SerialMQTT::connect() {
  MQTTMessage msg;

  // if (rxtx.available()) {
  //   rxtx.rxObj(msg);
  //
  //     if (msg.type == MQTTMessageType::CONNECT_ACK) {
  //       this->_isConnected = true;
  //       return *this;
  //     }
  //   }

  if (millis() - start_time > 1000) {
    Serial.printf("[mqtt] ||| serial disconnected: %lu\n", start_time);
    msg.type = MQTTMessageType::CONNECT;
    strcpy(msg.topic, "CONNECT");
    strcpy(msg.message, "");

    rxtx.sendDatum(msg);
    start_time = millis();
  }

  return *this;
}

void SerialMQTT::loop() {
  // EVERY_N_SECONDS(2) { Serial.println("[mqtt] inside serial mqtt loop."); }
  MQTTMessage msg;

  if (rxtx.available()) {
    rxtx.rxObj(msg);
    switch (msg.type) {
      case MQTTMessageType::MESSAGE: {
        Serial.printf("[mqtt] received topic: %s, message: %s\n", msg.topic,
                      msg.message);

        for (auto func : this->_onMessageList) {
          if (func != nullptr) {
            func(std::string{msg.topic}, std::string{msg.message});
          }
        }

        break;
      }

      case MQTTMessageType::CONNECT_ACK: {
        this->handleConnection();
        break;
      }

      case MQTTMessageType::STATUS_OK: {
        this->_lastHeartbeat = millis();
        this->_isConnected = true;
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
SerialMQTT& SerialMQTT::onMessage(CallbackOnMessage _c) {
  this->_onMessageList.push_back(_c);
  return *this;
}

SerialMQTT& SerialMQTT::onReady(CallbackOnReady _c) {
  this->_onReadyList.push_back(_c);
  return *this;
}

SerialMQTT& SerialMQTT::onDisconnect(CallbackOnDisconnect _c) {
  this->_onDisconnectList.push_back(_c);
  return *this;
}

SerialMQTT& SerialMQTT::onError(CallbackOnError _c) {
  this->_onErrorList.push_back(_c);
  return *this;
}

// ==========================================================================
// MQTT passthrough functions.
// ==========================================================================
bool SerialMQTT::publish(std::string topic, std::string message) {
  MQTTMessage msg;
  msg.type = MQTTMessageType::MESSAGE;
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

uint32_t SerialMQTT::getHeartbeatAge() {
  return (millis() - this->_lastHeartbeat);
}

bool SerialMQTT::connected() {
  return _isConnected;
}

void SerialMQTT::checkConnection() {
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

void SerialMQTT::handleConnection() {
  this->_isConnected = true;
  this->_lastHeartbeat = millis();

  Serial.println("[mqtt] ||| serial connected.");

  for (auto func : this->_onReadyList) {
    if (func != nullptr) {
      func();
    }
  }
}

#endif