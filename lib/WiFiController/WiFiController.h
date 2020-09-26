/**
 * Class for managing the wifi connection
 */

#ifndef WiFiController_h
#define WiFiController_h

#include <Arduino.h>
#include <WiFi.h>
// #include <WiFiClientSecure.h>
#include <LedshelfConfig.h>

class WiFiController
{
public:
    WiFiController(LedshelfConfig &c) : config(c)
    {
        WiFi.disconnect(true);
    };

    void setup();
    void connect();
    void testOutput();
    // WiFiClientSecure &getWiFiClient();
    WiFiClient &getWiFiClient();

private:
    // WiFiClientSecure wifiClient;
    WiFiClient wifiClient;
    LedshelfConfig &config;

    void handleEvent(WiFiEvent_t event);
};

#endif // WiFiController_h