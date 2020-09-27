
#include "MQTTController.h"
#include <FastLED.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

// MQTTController::MQTTController(const char *v_, LedshelfConfig &c_, WiFiController &w_, LightStateController &l_, Effects &e_)
// {
//     Serial.println("paramter constructor called");
// }

// MQTTController::MQTTController()
// {
//     Serial.println("Default constructor called");
// }

void MQTTController::setup()
{
    Serial.printf("Setting up MQTT Client: %s %i\n", config.mqtt_server, config.mqtt_port);

    config.statusTopic(statusTopic);
    config.commandTopic(commandTopic);
    config.updateTopic(updateTopic);
    config.stateTopic(stateTopic);
    config.queryTopic(queryTopic);
    config.informationTopic(informationTopic);

    timeClient.begin();
    Serial.println("NTP time client started.");

    WiFiClient &wifi = wifiCtrl.getWiFiClient();
    Serial.println("Got wifi client");

    client.setClient(wifi);
    client.setServer(config.mqtt_server, config.mqtt_port);
    client.setCallback([this](char *p_topic, byte *p_message, unsigned int p_length) {
        callback(p_topic, p_message, p_length);
    });
}

void MQTTController::checkConnection()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Restarting because WiFI not connected.");
        //delay(5000);
        ESP.restart();
        return;
    }
    if (!client.connected())
    {
        Serial.printf("MQTT broker not connected: %s\n", config.mqtt_server);
        connect();
    }

    client.loop();
}

void MQTTController::publishState()
{
    char json[256];
    lightState.serializeCurrentState(json, 256);

    client.publish(stateTopic, json, true);
}

void MQTTController::publishInformation(const char *message)
{
    client.publish(informationTopic, message, false);
}

void MQTTController::publishInformationData()
{
    char message[256];
    char *msg = message;

    msg += sprintf(msg, "\"time\": \"%s\", ", timeClient.getFormattedTime().c_str());
    msg += sprintf(msg, "\"hostname\": \"%s\", ", WiFi.getHostname());
    msg += sprintf(msg, "\"version\": \"%s\", ", version);
    msg += sprintf(msg, "\"uptime\": %d, ", millis());
    msg += sprintf(msg, "\"memory\": %d }", xPortGetFreeHeapSize());

    publishInformation(message);
}

/**
 * Callback when we recive MQTT messages on topics we listen to.
 *
 * Parses message, dispatches commands and updates settings.
 */
void MQTTController::callback(char *p_topic, byte *p_message, unsigned int p_length)
{
    Serial.printf("- MQTT Got topic: '%s'\n", p_topic);
    if (effects.currentCommandType == Effects::Command::FirmwareUpdate)
    {
        publishInformation("Firmware update active. Ignoring command.");
        return;
    }

    if (strcmp(commandTopic, p_topic) == 0)
    {

        // LightState& newState = lightState.parseNewState(p_message);
        handleNewState(lightState.parseNewState(p_message));
        publishState();
        return;
    }

    if (strcmp(updateTopic, p_topic) == 0)
    {
        handleUpdate();
        return;
    }

    Serial.printf("- ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
    return;
}
// End of MQTT Callback

/**
 * Looks over the new state and dispatches updates to effects and issues
 * commands.
 */
void MQTTController::handleNewState(LightState &state)
{
    if (state.state == false)
    {
        Serial.println("- Told to turn off");
        FastLED.setBrightness(0);
        return;
    }

    if (state.status.hasBrightness)
    {
        Serial.printf("- Got new brightness: '%i'\n", state.brightness);
        effects.setCurrentCommand(Effects::Command::Brightness);
    }
    else if (state.status.hasColor)
    {
        Serial.println("  - Got color");
        if (effects.getCurrentEffect() == Effects::Effect::NullEffect)
        {
            effects.setCurrentCommand(Effects::Command::Color);
        }
        else
        {
            Serial.printf(
                "    - Effect is: '%s' hue: %.2f\n",
                state.effect.c_str(), state.color.h);
            effects.setStartHue(state.color.h);
        }
    }
    else if (state.status.hasEffect)
    {
        Serial.printf("  - Got effect '%s'. Setting it.\n", state.effect.c_str());
        effects.setCurrentEffect(state.effect);
        if (state.effect == "")
        {
            effects.setCurrentCommand(Effects::Command::Color);
        }
    }
    else if (state.status.hasColorTemp)
    {
        unsigned int kelvin = (1000000 / state.color_temp);
        Serial.printf("  - Got color temp: %i mired = %i Kelvin\n", state.color_temp, kelvin);

        unsigned int temp = kelvin / 100;

        double red = 0;
        if (temp <= 66)
        {
            red = 255;
        }
        else
        {
            red = temp - 60;
            red = 329.698727446 * (pow(red, -0.1332047592));
            if (red < 0)
                red = 0;
            if (red > 255)
                red = 255;
        }

        double green = 0;
        if (temp <= 66)
        {
            green = temp;
            green = 99.4708025861 * log(green) - 161.1195681661;
            if (green < 0)
                green = 0;
            if (green > 255)
                green = 255;
        }
        else
        {
            green = temp - 60;
            green = 288.1221695283 * (pow(green, -0.0755148492));
            if (green < 0)
                green = 0;
            if (green > 255)
                green = 255;
        }

        double blue = 0;
        if (temp >= 66)
        {
            blue = 255;
        }
        else
        {
            if (temp <= 19)
            {
                blue = 0;
            }
            else
            {
                blue = temp - 10;
                blue = 138.5177312231 * log(blue) - 305.0447927307;
                if (blue < 0)
                    blue = 0;
                if (blue > 255)
                    blue = 255;
            }
        }

        Serial.printf(
            "    - RGB [%i, %i, %i]\n",
            static_cast<uint8_t>(red),
            static_cast<uint8_t>(green),
            static_cast<uint8_t>(blue));
        state.color.r = static_cast<uint8_t>(red);
        state.color.g = static_cast<uint8_t>(green);
        state.color.b = static_cast<uint8_t>(blue);

        effects.setCurrentCommand(Effects::Command::Color);
    }
    else
    {
        // assuming turn on is the only thing.
        effects.setCurrentCommand(Effects::Command::Brightness);
    }
}

void MQTTController::handleUpdate()
{
    publishInformation("Got update notification. Getting ready to perform firmware update.");
    Serial.println("Running ArduinoOTA");
    ArduinoOTA.begin();

    effects.setCurrentEffect(Effects::Effect::NullEffect);
    effects.setCurrentCommand(Effects::Command::FirmwareUpdate);

    effects.runCurrentCommand();
}

void MQTTController::connect()
{
    IPAddress mqttip;
    WiFi.hostByName(config.mqtt_server, mqttip);

    Serial.printf("  - %s = ", config.mqtt_server);
    Serial.println(mqttip);

    while (!client.connected())
    {
        Serial.printf(
            "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\":\"%s\" ...\n",
            config.mqtt_server, config.mqtt_port, config.mqtt_username, config.mqtt_password);

        Serial.printf("  - statusTopic: %s\n", statusTopic);

        // if (client.connect(
        //         config.mqtt_client,
        //         config.mqtt_username,
        //         config.mqtt_password,
        //         statusTopic,
        //         0, true, "Disconnected"))
        if (client.connect(config.mqtt_client, config.mqtt_username, config.mqtt_password, statusTopic, 0, true, "Disconnected"))
        {
            Serial.println(" connected");
            Serial.printf("  - status:  '%s'\n", statusTopic);
            Serial.printf("  - command: '%s'\n", commandTopic);
            Serial.printf("  - update: '%s'\n", updateTopic);

            timeClient.update();

            client.subscribe(commandTopic);
            client.subscribe(queryTopic);
            client.subscribe(updateTopic);

            publishStatus();
            publishInformationData();
        }
        else
        {
            Serial.print(" failed: ");
            Serial.println(client.state());
            delay(5000);
        }
    }

    publishState();
}

void MQTTController::publishStatus()
{
    client.publish(statusTopic, "Online", true);
}

// Trying to create a global object