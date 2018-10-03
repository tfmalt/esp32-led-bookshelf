/**
 * Class for managing the wifi connection
 */

#ifndef WiFiController_h
#define WiFiController_h

#include <Arduino.h>
#include <WiFiClientSecure.h>

class WiFiController {
    public:
        WiFiController()
        {
            WiFi.disconnect(true);
        };

        void                setup(const char* ssid_i, const char* psk_i, const char* ca);
        void                connect();
        void                testOutput();
        WiFiClientSecure&   getWiFiClient();

    private:
        WiFiClientSecure    wifiClient;
        const char*         ssid;
        const char*         psk;

        void handleEvent(WiFiEvent_t event);
};

#endif // WiFiController_h