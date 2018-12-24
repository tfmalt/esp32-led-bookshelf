
#ifndef MQTTController_h
#define MQTTController_h

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiController.h>
#include <LightStateController.h>
#include <LedshelfConfig.h>
#include <Effects.h>

class MQTTController {
    public:
        MQTTController();

        void setup(
            String v,
            WiFiController* wc,
            LightStateController* lc,
            LedshelfConfig* c,
            Effects *e
        );
        void checkConnection();
        void publishInformation(const char* message);
        void publishInformationData();
        void publishStatus();

    private:
        String                  version;
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