
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

class MQTTController
{
    const char *version;
    LedshelfConfig &config;
    PubSubClient client;
    WiFiController &wifiCtrl;
    LightStateController &lightState;
    Effects &effects;

public:
    // MQTTController();
    MQTTController(
        const char *v_,
        LedshelfConfig &c_,
        WiFiController &w_,
        LightStateController &l_,
        Effects &e_) : version(v_), config(c_), wifiCtrl(w_), lightState(l_), effects(e_){};

    void setup();
    void checkConnection();
    void publishInformation(const char *message);
    void publishInformationData();
    void publishStatus();

private:
    void publishState();
    void handleNewState(LightState &state);
    void handleUpdate();
    void callback(char *p_topic, byte *p_message, unsigned int p_length);
    void connect();
};

#endif // MQTTController_h