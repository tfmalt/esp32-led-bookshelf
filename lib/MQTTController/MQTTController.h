
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
#include <functional>

class MQTTController {
    public:
        typedef std::function<void(char*, byte*, unsigned int)> CallbackFunction;

        MQTTController();

        void setup(
            String v,
            WiFiController* wc,
            LedshelfConfig* c,
            CallbackFunction fn
        );
        void checkConnection();
        void publishInformation(const char* message);
        void publishStatus();
        void publishState(LightStateController&);
        void connect();

    private:
        String                  version;
        PubSubClient            client;
        WiFiController*         wifiCtrl;
        LedshelfConfig*         config;
        CallbackFunction        onCallback;


        void handleNewState(LightState& state);
        void handleUpdate();
};

#endif // MQTTController_h