#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <Arduino.h>
#include <FastLED.h>

#include <LedshelfConfig.hpp>
#include <map>
#include <string>

#ifdef IS_ESP32
#include <MQTTController.hpp>
#endif  // IS_ESP32

#ifdef IS_TEENSY
#include <SerialMQTT.hpp>
#endif

typedef std::map<std::string, std::function<void(std::string, std::string)>>
    TopicHandlerMap;

class EventDispatcher {
 public:
  EventDispatcher(){};

  void setup() {
    mqtt.setup();
    mqtt.onMessage(handleMessage);
    mqtt.onReady(handleReady);
    mqtt.onDisconnect(handleDisconnect);
    mqtt.onError(handleError);
  }

  void publishInformation(const char *message) {
    mqtt.publishInformation(message);
  };

  void publishInformation(const std::string message) {
    mqtt.publishInformation(message.c_str());
  };

  void publishInformation() {
    // to replace informationdata
    mqtt.publishInformationData();
  }

  void publishStatus() {
    // make this more general
    mqtt.publishStatus();
  }

  void loop() {
    mqtt.checkConnection();

    EVERY_N_SECONDS(60) {
      publishStatus();
      publishInformation();  // todo: clean up this
    }
  }

 private:
  static LedshelfConfig config;
  static TopicHandlerMap handlers;
#ifdef IS_ESP32
  MQTTController mqtt;
#endif
#ifdef IS_TEENSY
  SerialMQTT mqtt;
#endif

  static void handleMessage(std::string topic, std::string message) {
    Serial.println("Handle Message:");
    Serial.printf("  - topic: %s\n", topic.c_str());
    Serial.println(message.c_str());

    handlers[config.state_topic] = handleState;

    auto h = handlers.find(topic);
    if (h != handlers.end()) {
      h->second(topic, message);
    }
  }

  static void handleReady() {
    // got ready handler
    Serial.println("GOT MQTT Ready");
  }

  static void handleDisconnect(std::string msg) {
    Serial.printf("MQTT Disconnect: %s\n", msg.c_str());
  }

  static void handleError(std::string error) {
    Serial.printf("ERROR: %s\n", error.c_str());
  }

  static void handleState(std::string topic, std::string message) {
    Serial.printf("Handle State: %s\n", message.c_str());
  }
};

#endif  // EVENTDISPATCHER_HPP