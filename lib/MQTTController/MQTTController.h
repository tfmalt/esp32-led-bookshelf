
#ifndef MQTTController_h
#define MQTTController_h

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WiFiController.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>
#include <Effects.h>

class MQTTController {
    public:
        MQTTController();

        void setup(
            WiFiController* wc,
            LightStateController* lc,
            LedshelfConfig* c,
            Effects* e
        );
        void checkConnection();
    private:
        PubSubClient            client;
        WiFiController*         wifiCtrl;
        LightStateController*   lightState;
        LedshelfConfig*         config;
        Effects*                effects;

        void publishState();
        void handleNewState(LightState& state);
        void callback(char* p_topic, byte* p_message, unsigned int p_length);
        void connect();
};

#endif // MQTTController_h