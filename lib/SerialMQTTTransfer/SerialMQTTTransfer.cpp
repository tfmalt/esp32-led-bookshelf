#ifdef TEENSY
#include "SerialMQTTTransfer.h"
#include <Arduino.h>

#define SERIAL_BAUD_RATE 57600
uint32_t start_time = 0;

SerialMQTTTransfer::SerialMQTTTransfer(){};

void SerialMQTTTransfer::begin() {
  if (verbose()) {
    Serial.println("[mqtt] running setup for SerialMQTTTransfer.");
  }
  Serial3.begin(RXTX_BAUD_RATE, SERIAL_8N1);
  rxtx.begin(Serial3);
}

bool SerialMQTTTransfer::connect() {
  MQTTMessage msg;
  // Serial.printf("debug: %u, %u\n", millis(), start_time);
  if (millis() - start_time > 1000) {
    Serial.printf("[mqtt] ||| serial disconnected: %lu\n", getHeartbeatAge());
    msg.type = MQTTMessageType::CONNECT;
    strcpy(msg.topic, "CONNECT");
    strcpy(msg.message, "");

    rxtx.sendDatum(msg);
    start_time = millis();
  }

  return true;
}

void SerialMQTTTransfer::loop() {
  Serial.println("mqtt transfer loop");
  if (rxtx.available()) {
    MQTTMessage msg;
    // msg.count = 0;
    rxtx.rxObj(msg);
    Serial.println("Got transfer");
    this->_lastHeartbeat = millis();
    switch (msg.type) {
      case MQTTMessageType::MESSAGE: {
        if (verbose()) {
          Serial.printf("[mqtt] received topic: %s, message: %s\n", msg.topic,
                        msg.message);
        }

        this->emitMessage(std::string{msg.topic}, std::string{msg.message});
        break;
      }

      case MQTTMessageType::CONNECT_ACK: {
        Serial.println("got connect_ack");
        this->handleConnection();
        break;
      }

      case MQTTMessageType::STATUS_OK: {
        if (verbose()) {
          Serial.printf("[mqtt] <<< STATUS OK: count: %i\n", msg.count);
        }
        // this->_isConnected = true;
        if (msg.count < 2) {
          this->emitMissingSubscribe();
        }
        break;
      }

      case MQTTMessageType::STATUS_NO_SUB: {
        if (verbose()) {
          Serial.printf("[mqtt] <<< NO SUBSCRIPTIONS: %i\n", msg.count);
        }
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
SerialMQTTTransfer& SerialMQTTTransfer::onMissingSubscribe(OnReadyFunction _c) {
  this->_onMissingSubscribeList.push_back(_c);
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

// ==========================================================================
// MQTT passthrough functions.
// ==========================================================================
bool SerialMQTTTransfer::subscribe(std::string topic) {
  MQTTMessage msg;
  msg.type = MQTTMessageType::SUBSCRIBE;
  strcpy(msg.topic, topic.c_str());
  strcpy(msg.message, "");

  rxtx.sendDatum(msg);

  return true;
}

bool SerialMQTTTransfer::subscribe(const char* topic) {
  return subscribe(std::string{topic});
}

bool SerialMQTTTransfer::publish(std::string topic, std::string message) {
  MQTTMessage msg;
  msg.type = MQTTMessageType::MESSAGE;
  strcpy(msg.topic, topic.c_str());
  strcpy(msg.message, message.c_str());
  rxtx.sendDatum(msg);

  return true;
}

bool SerialMQTTTransfer::publish(const char* topic, const char* message) {
  return publish(std::string{topic}, std::string{message});
}

void SerialMQTTTransfer::publishInformationData() {
  if (verbose()) {
    Serial.println("DATA");
  }
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
    // Serial.println("[mqtt] ||| Not connected. retrying");
    this->connect();
  }
}

void SerialMQTTTransfer::handleConnection() {
  this->_isConnected = true;
  this->_lastHeartbeat = millis();

  if (verbose()) {
    Serial.println("[mqtt] ||| serial connected.");
  }

  this->emitReady();
}

#endif