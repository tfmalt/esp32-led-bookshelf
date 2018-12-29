
#ifndef MQTTController_h
#define MQTTController_h

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <WiFiController.h>
#include <LightState.h>
#include <Light.h>
#include <LedshelfConfig.h>
#include <Effects.h>
#include <functional>

class MQTTController {
    public:
        typedef std::function<void(char*, byte*, unsigned int)> CallbackFunction;

        MQTTController();

        void setup(
            WiFiController* wc,
            LedshelfConfig* c,
            CallbackFunction fn
        );
        void checkConnection();
        void publishInformation(const char* message);
        void publishStatus();
        void publishState(Light& light);
        void connect();
        void subscribe(const char* topic);

    private:
        PubSubClient            client;
        WiFiController*         wifiCtrl;
        LedshelfConfig*         config;
        CallbackFunction        onCallback;


        void handleNewState(LightState& state);
        void handleUpdate();
};

#endif // MQTTController_h