
#include "MQTTController.h"
#include <FastLED.h>
#include <WiFiUdp.h>
#include <functional>


MQTTController::MQTTController()
{
}

void MQTTController::setup(
    String v,
    WiFiController* wc,
    LedshelfConfig* c,
    CallbackFunction fn
) {
    wifiCtrl   = wc;
    config     = c;
    version    = v;
    onCallback = fn;

    Serial.printf("Setting up MQTT Client: %s %i\n", config->server.c_str(), config->port);


    client.setClient(wifiCtrl->getWiFiClient());
    client.setServer(config->server.c_str(), config->port);
    client.setCallback(onCallback);
}

void MQTTController::checkConnection()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Restarting because WiFI not connected.");
        //delay(5000);
        ESP.restart();
        return;
    }
    if (!client.connected()) {
        Serial.printf("MQTT broker not connected: %s\n", config->server.c_str());
        connect();
    }

    client.loop();
}

void MQTTController::publishState(LightStateController& state)
{
    char json[256];
    state.printStateJsonTo(json);
    client.publish(config->stateTopic().c_str(), json, true);
}

void MQTTController::publishInformation(const char* message)
{
    client.publish(config->informationTopic().c_str(), message, false);
}

void MQTTController::connect()
{
    IPAddress mqttip;
    WiFi.hostByName(config->server.c_str(), mqttip);

    Serial.printf("  - %s = ", config->server.c_str());
    Serial.println(mqttip);

    while (!client.connected()) {
        Serial.printf(
            "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\" ... ",
            config->server.c_str(), config->port, config->username.c_str()
        );

        if (client.connect(
            config->client.c_str(),
            config->username.c_str(),
            config->password.c_str(),
            config->statusTopic().c_str(),
            0, true, "Disconnected")
        ) {
            Serial.println(" connected");

            client.subscribe(config->commandTopic().c_str());
            client.subscribe(config->queryTopic().c_str());
            client.subscribe(config->updateTopic().c_str());
        }
        else {
            Serial.print(" failed: ");
            Serial.println(client.state());
            delay(5000);
        }
    }
}

void MQTTController::publishStatus()
{
    client.publish(config->statusTopic().c_str(), "Online", true);
}

// Trying to create a global object