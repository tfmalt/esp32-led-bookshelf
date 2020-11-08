#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <Arduino.h>
#include <FastLED.h>

#include <LedshelfConfig.hpp>
#include <string>

#ifdef IS_ESP32
#include <MQTTController.hpp>
#endif  // IS_ESP32

#ifdef IS_TEENSY
#include <SerialMQTT.hpp>
#endif

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
  LedshelfConfig config;
#ifdef IS_ESP32
  MQTTController mqtt;
#endif
#ifdef IS_TEENSY
  SerialMQTT mqtt;
#endif

  static void handleMessage(std::string topic, std::string message) {
    Serial.println("Got data:");
    Serial.println(topic.c_str());
    Serial.println(message.c_str());
  }

  static void handleReady() { Serial.println("GOT MQTT Ready"); }
  static void handleDisconnect() { Serial.println("MQTT client disconnected"); }
  static void handleError(std::string error) {
    Serial.printf("GOT ERROR: %s\n", error.c_str());
  }
};

#endif  // EVENTDISPATCHER_HPP