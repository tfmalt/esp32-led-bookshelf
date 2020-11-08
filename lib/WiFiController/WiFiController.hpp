/**
 * Class for managing the wifi connection
 */

#ifndef WiFiController_h
#define WiFiController_h

#include <Arduino.h>
#ifdef IS_ESP32
#include <WiFi.h>
#endif
// #include <WiFiClientSecure.h>
#include <LedshelfConfig.hpp>

class WiFiController {
 public:
  WiFiController() { WiFi.disconnect(true); };

  void setup();
  void connect();
  void testOutput();
  // WiFiClientSecure &getWiFiClient();
  WiFiClient &getWiFiClient();

 private:
  // WiFiClientSecure wifiClient;
  WiFiClient wifiClient;
  LedshelfConfig config;

  void handleEvent(WiFiEvent_t event);
};

#endif  // WiFiController_h