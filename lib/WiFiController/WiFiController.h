/**
 * Class for managing the wifi connection
 */

#ifndef WiFiController_h
#define WiFiController_h

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <LedshelfConfig.h>

class WiFiController {
    public:
        WiFiController()
        {
            WiFi.disconnect(true);
        };

        void                setup(LedshelfConfig* c);
        void                connect();
        void                testOutput();
        WiFiClientSecure&   getWiFiClient();

    private:
        WiFiClientSecure    wifiClient;
        LedshelfConfig*     config;

        void handleEvent(WiFiEvent_t event);
};

#endif // WiFiController_h