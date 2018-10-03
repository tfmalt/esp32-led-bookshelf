/**
 * Class for managing the wifi connection
 */

#ifndef WiFiController_h
#define WiFiController_h

#include <Arduino.h>
#include <WiFiClientSecure.h>

class WiFiController {
    public:
        WiFiController() :
            ssid(ssid_p),
            psk(psk_p)
        {
            WiFi.disconnect(true);
        };

        void                setup(String ssid_i, String psk_i);
        void                connect();
        void                testOutput();
        WiFiClientSecure&   getWiFiClient();

        const String &ssid;
        const String &psk;

    private:
        WiFiClientSecure    wifiClient;
        String         ssid_p;
        String         psk_p;

        // typedef void (WiFiController::*EventHandler)(WiFiEvent_t event);
        // EventHandler eventHandler;
        void handleEvent(WiFiEvent_t event);
};

#endif // WiFiController_h