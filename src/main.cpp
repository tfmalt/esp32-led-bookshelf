/*
 * A first attempt at writing code for ESP32.
 * This is going to be a led light controller for lights on my daughters book shelf.
 *
 * - Uses MQTT over TLS to talk to a MQTT broker.
 * - Implementes the mqtt_json interface to get commands from a home assistant
 *   server. (https://home-assistant.io)
 * - Uses SPIFFS to store configuration and separate configuration from firmware
 *
 * MIT License
 *
 * Copyright (c) 2018 Thomas Malt
 */

#define DEBUG 1

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <WiFiController.h>
#include <FastLED.h>
#include <NTPClient.h>
#include <map>
#include <Effects.h>
#include <MQTTController.h>
#include <LedshelfConfig.h>
#include <LightState.h>
#include <Light.h>

FASTLED_USING_NAMESPACE

static const String VERSION = "v0.3.7";

// Fastled definitions
static const uint8_t GPIO_DATA         = 18;

// 130 bed lights
// 384 shelf lights
static const uint16_t NUM_LEDS         = 130;
static const uint8_t FPS               = 60;
static const uint8_t FASTLED_SHOW_CORE = 0;

// global objects
CRGBArray<NUM_LEDS>   leds;
LedshelfConfig        config;         // read from json config file.
LightState  lightState;     // Own object. Responsible for state.
WiFiController        wifiCtrl;
MQTTController        mqttCtrl; // This object is created in library.
Effects               effects;
WiFiUDP               ntpUDP;
NTPClient             timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);
std::map<String, Light>    lights;

uint16_t commandFrames     = FPS;
uint16_t commandFrameCount = 0;
ulong    commandStart      = 0;
uint8_t  updateProgress    = 0;

void publishInformationData()
{
    String message =
        "{\"time\": \"" + timeClient.getFormattedTime() +"\", " +
        "\"hostname\": \"" + String(WiFi.getHostname()) + "\", " +
        "\"ip\": \"" + String(WiFi.localIP().toString()) + "\", " +
        "\"version\": \"" + VERSION + "\", " +
        "\"uptime\": " + millis() + ", " +
        "\"memory\": " + xPortGetFreeHeapSize() +
        "}";

    mqttCtrl.publishInformation(message.c_str());
}

void setupFastLED()
{
    Serial.println("Setting up LED");
    Serial.printf("  - number of leds: %i\n", NUM_LEDS);
    Serial.printf("  - maximum milliamps: %i\n", config.milliamps);
    Serial.printf("  - number of separate lights: %i\n", config.lights.size());

    // lightState.initialize();
    // LightStateData& currentState = lightState.getCurrentState();

    // Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(UncorrectedColor);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, config.milliamps);

    LightConfig allConf = {
        String("light"),
        {0, NUM_LEDS-1},
        {NUM_LEDS-1, 0},
        config.command_topic,
        config.state_topic
    };

    // const char* username = config.username.c_str();
    lights["light"] = Light(leds, FPS, allConf, config.username);
    for (LightConfig item : config.lights) {
        lights[item.name] = Light(leds, FPS, item, config.username);
    }

    uint8_t brightness = lights["light"].getBrightness();
    Serial.printf("  - initial brightness: %i\n", brightness);
    FastLED.setBrightness(brightness);

    // effects.setFPS(FPS);
    // effects.setLightState(&lightState);
    // effects.setLeds(leds, NUM_LEDS);
    // effects.setCurrentEffect(currentState.effect);
    // effects.setStartHue(currentState.color.h);

    mqttCtrl.publishStatus();
    for(auto & item : lights) {
        mqttCtrl.publishState(item.second);
    }
    publishInformationData();
}

void setupArduinoOTA()
{
    Serial.println("Setting up ArduinoOTA.");

    ArduinoOTA.setPort(3232);
    ArduinoOTA.setPassword(config.password.c_str());
    ArduinoOTA
        .onStart([]() {
            // U_FLASH or U_SPIFFS
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            mqttCtrl.publishInformation((String("ArduinioOTA started updating " + type)).c_str());
        })
        .onEnd([]() {
            mqttCtrl.publishInformation("ArduinoOTA Finished updating");
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            String errmsg;
            switch (error) {
                case OTA_AUTH_ERROR:
                    errmsg = "Authentication failed";
                    break;
                case OTA_BEGIN_ERROR:
                    errmsg = "Begin failed";
                    break;
                case OTA_CONNECT_ERROR:
                    errmsg = "Connect failed";
                    break;
                case OTA_RECEIVE_ERROR:
                    errmsg = "Receive failed";
                    break;
                case OTA_END_ERROR:
                    errmsg = "End failed";
                    break;
                default:
                    errmsg = "Unknown error";
            }
            String message = "ArduinoOTA Firmware Update Error [" + String(error) + "]: " + errmsg;
            mqttCtrl.publishInformation(message.c_str());
        });
}

/**
 * Looks over the new state and dispatches updates to effects and issues
 * commands.
 */
void handleNewState(LightStateData& state) {
    if (state.state == false) {
        FastLED.setBrightness(0);
        return;
    }

    if(state.status.hasBrightness)
    {
        Serial.printf("- Got new brightness: '%i'\n", state.brightness);
        effects.setCurrentCommand(Effects::Command::Brightness);
    }
    else if(state.status.hasColor) {
        Serial.println("  - Got color");
        if (effects.getCurrentEffect() == Effects::Effect::NullEffect) {
            effects.setCurrentCommand(Effects::Command::Color);
        }
        else {
            effects.setStartHue(state.color.h);
        }
    }
    else if (state.status.hasEffect) {
        Serial.printf("  - Got effect '%s'. Setting it.\n", state.effect.c_str());
        effects.setCurrentEffect(state.effect);
        if (state.effect == "") {
            effects.setCurrentCommand(Effects::Command::Color);
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

        effects.setCurrentCommand(Effects::Command::Color);
    }
    else {
        // assuming turn on is the only thing.
        effects.setCurrentCommand(Effects::Command::Brightness);
    }
}


void handleOTAUpdate()
{
    mqttCtrl.publishInformation("Got update notification. Getting ready to perform firmware update.");
    Serial.println("Running ArduinoOTA");
    ArduinoOTA.begin();

    effects.setCurrentEffect(Effects::Effect::NullEffect);
    effects.setCurrentCommand(Effects::Command::FirmwareUpdate);
    effects.runCurrentCommand();
}

void mqttCallback(char* topic, byte* message, unsigned int length)
{
    Serial.printf("- MQTT Got topic: '%s'\n", topic);

    if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
        mqttCtrl.publishInformation("Firmware update active. Ignoring command.");
        return;
    }

    if (config.commandTopic().equals(topic)) {
        handleNewState(lightState.parseNewState(message));
        // mqttCtrl.publishState(lightState);
        return;
    }

    if (config.updateTopic().equals(topic)) {
        handleOTAUpdate();
        return;
    }

    Serial.printf("- ERROR: Not a valid topic: '%s'. IGNORING\n", topic);
    return;
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting version %s...\n", VERSION.c_str());

    config.setup();
    wifiCtrl.setup(&config);
    mqttCtrl.setup(&wifiCtrl, &config, mqttCallback);

    wifiCtrl.connect();
    mqttCtrl.connect();
    timeClient.begin();
    timeClient.update();

    setupArduinoOTA();
    Serial.println("Waiting for LEDs ...");
    delay(3000);
    setupFastLED();
}

void loop() {
    mqttCtrl.checkConnection();

    if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
         ArduinoOTA.handle();
         if (millis() > (effects.commandStart + 180000)) {
             mqttCtrl.publishInformation("No update started for 180s. Rebooting.");
             ESP.restart();
         }
    } else {

        effects.runCurrentCommand();
        effects.runCurrentEffect();

        EVERY_N_SECONDS(60) {
            mqttCtrl.publishStatus();
            publishInformationData();
        }
        // fastLEDshowESP32();
        FastLED.show();
        delay(1000/FPS);
    }
}

