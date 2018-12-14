
#include "MQTTController.h"
#include <FastLED.h>

MQTTController::MQTTController()
{

}

void MQTTController::setup(
    WiFiController* wc,
    LightStateController* lc,
    LedshelfConfig* c,
    Effects* e
) {
    wifiCtrl   = wc;
    lightState = lc;
    config     = c;
    effects    = e;

    Serial.printf("Setting up MQTT Client: %s %i\n", config->server.c_str(), config->port);

    client.setClient(wifiCtrl->getWiFiClient());
    client.setServer(config->server.c_str(), config->port);
    client.setCallback([this](char* p_topic, byte* p_message, unsigned int p_length) {
        callback(p_topic, p_message, p_length);
    });
}

void MQTTController::checkConnection()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Skipping MQTT because WiFI not connected.");
        delay(500);
        return;
    }
    if (!client.connected()) {
        Serial.printf("MQTT broker not connected: %s\n", config->server.c_str());
        connect();
    }

    client.loop();
}

void MQTTController::publishState()
{
    char json[256];
    lightState->printStateJsonTo(json);
    client.publish(config->state_topic.c_str(), json, true);
}

/**
 * Callback when we recive MQTT messages on topics we listen to.
 *
 * Parses message, dispatches commands and updates settings.
 */
void MQTTController::callback (char* p_topic, byte* p_message, unsigned int p_length)
{
    if (!config->command_topic.equals(p_topic)) {
        Serial.printf("- ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
        return;
    }

    // LightState& newState = lightState->parseNewState(p_message);
    handleNewState(lightState->parseNewState(p_message));
    publishState();
}
// End of MQTT Callback

/**
 * Looks over the new state and dispatches updates to effects and issues
 * commands.
 */
void MQTTController::handleNewState(LightState& state) {
    if (state.state == false) {
        Serial.println("- Told to turn off");
        FastLED.setBrightness(0);
        return;
    }

    if(state.status.hasBrightness)
    {
        Serial.printf("- Got new brightness: '%i'\n", state.brightness);
        effects->setCurrentCommand(Effects::Command::Brightness);
    }
    else if(state.status.hasColor) {
        Serial.println("  - Got color");
        if (effects->getCurrentEffect() == Effects::Effect::NullEffect) {
            effects->setCurrentCommand(Effects::Command::Color);
        }
        else {
            Serial.printf(
                "    - Effect is: '%s' hue: %.2f\n",
                state.effect.c_str(), state.color.h
            );
            effects->setStartHue(state.color.h);
        }
    }
    else if (state.status.hasEffect) {
        Serial.printf("  - Got effect '%s'. Setting it.\n", state.effect.c_str());
        effects->setCurrentEffect(state.effect);
        if (state.effect == "") {
            effects->setCurrentCommand(Effects::Command::Color);
        }
    }
    else if (state.status.hasColorTemp) {
        unsigned int kelvin = (1000000/state.color_temp);
        Serial.printf("  - Got color temp: %i mired = %i Kelvin\n", state.color_temp, kelvin);

        unsigned int temp = kelvin / 100;

        double red = 0;
        if (temp <= 66) {
            red = 255;
        }
        else {
            red = temp - 60;
            red = 329.698727446 * (pow(red, -0.1332047592));
            if (red < 0)    red = 0;
            if (red > 255)  red = 255;
        }

        double green = 0;
        if (temp <= 66) {
            green = temp;
            green = 99.4708025861 * log(green) - 161.1195681661;
            if (green < 0)      green = 0;
            if (green > 255)    green = 255;
        }
        else {
            green = temp - 60;
            green = 288.1221695283 * (pow(green, -0.0755148492));
            if (green < 0)      green = 0;
            if (green > 255)    green = 255;
        }

        double blue = 0;
        if (temp >= 66) {
            blue = 255;
        }
        else {
            if (temp <= 19) {
                blue = 0;
            }
            else {
                blue = temp - 10;
                blue = 138.5177312231 * log(blue) - 305.0447927307;
                if (blue < 0)   blue = 0;
                if (blue > 255) blue = 255;
            }
        }

        Serial.printf(
            "    - RGB [%i, %i, %i]",
            static_cast<uint8_t>(red),
            static_cast<uint8_t>(green),
            static_cast<uint8_t>(blue)
        );
        state.color.r = static_cast<uint8_t>(red);
        state.color.g = static_cast<uint8_t>(green);
        state.color.b = static_cast<uint8_t>(blue);

        effects->setCurrentCommand(Effects::Command::Color);
    }
    else {
        // assuming turn on is the only thing.
        effects->setCurrentCommand(Effects::Command::Brightness);
    }
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
            config->status_topic.c_str(),
            0, true, "Disconnected")
        ) {
            Serial.println(" connected");
            Serial.printf("  - status:  '%s'\n", config->status_topic.c_str());
            Serial.printf("  - command: '%s'\n", config->command_topic.c_str());
            client.publish(config->status_topic.c_str(), "Online", true);
            client.subscribe(config->command_topic.c_str());
        }
        else {
            Serial.print(" failed: ");
            Serial.println(client.state());
            delay(5000);
        }
    }

    publishState();
}
