
#ifndef MQTTController_h
#define MQTTController_h

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <LightStateController.h>
#include <WiFiClientSecure.h>
#include <WiFiController.h>
#include <LedshelfConfig.h>
#include <PubSubClient.h>
#include <Effects.h>

class MQTTController {
    public:
        MQTTController();

        void setup(
            WiFiController* wc,
            LightStateController* lc,
            LedshelfConfig* c,
            Effects *e
        );
        void checkConnection();
        void publishInformation(const char* message);
        void publishStatus();

    private:
        PubSubClient            client;
        WiFiController*         wifiCtrl;
        LightStateController*   lightState;
        LedshelfConfig*         config;
        Effects*                effects;

        void publishState();
        void handleNewState(LightState& state);
        void handleUpdate();
        void callback(char* p_topic, byte* p_message, unsigned int p_length);
        void connect();
};

#endif // MQTTController_h