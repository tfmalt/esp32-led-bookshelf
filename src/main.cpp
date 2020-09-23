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
 * Copyright (c) 2018-2019 Thomas Malt
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

static const String VERSION = "v0.3.10";

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
WiFiController        wifiCtrl;
MQTTController        mqttCtrl; // This object is created in library.
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

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(UncorrectedColor);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, config.milliamps);

    LightConfig allConf = {
        String("light"),
        {0, NUM_LEDS-1},
        {NUM_LEDS-1, 0},
        config.command_topic,
        config.state_topic
    };

    lights["light"] = Light(leds, NUM_LEDS, FPS, allConf, config.username);
    lights["light"].isMaster(true);
    for (LightConfig item : config.lights) {
        lights[item.name] = Light(leds, NUM_LEDS, FPS, item, config.username);
    }

    FastLED.setBrightness(lights["light"].getBrightness());

    mqttCtrl.publishStatus();
    for(auto & i : lights) {
        mqttCtrl.subscribe(i.second.getCommandTopic().c_str());
        mqttCtrl.publishState(i.second);
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

Light* getMasterLight()
{
    for (auto& i : lights) {
        if (i.second.isMaster()) return &i.second;
    }

    Serial.println("Reached end of getmasterlight");
    return nullptr;
}

void handleOTAUpdate()
{
    mqttCtrl.publishInformation("Got update notification. Getting ready to perform firmware update.");
    Serial.println("Running ArduinoOTA");

    Light* master = getMasterLight();

    master->setCurrentEffect(Effects::Effect::NullEffect);
    master->setCurrentCommand(Effects::Command::FirmwareUpdate);
    master->runCurrentCommand();

    ArduinoOTA.begin();
}

void mqttCallback(char* topic, byte* message, unsigned int length)
{
    Serial.printf("- MQTT Got topic: '%s'\n", topic);

    Light* master = getMasterLight();

    if (master->getCurrentCommand() == Effects::Command::FirmwareUpdate) {
        mqttCtrl.publishInformation("Firmware update active. Ignoring command.");
        return;
    }

    for (auto& i : lights) {
       if (i.second.getCommandTopic().equals(topic)) {
           Serial.printf("  - Found: %s\n", topic);
           i.second.onCommand(message);
           mqttCtrl.publishState(i.second);
           return;
       }
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

    Light* master = getMasterLight();
    // Serial.printf("Got here: %s\n", master->getName().c_str());
    if (master->getCurrentCommand() == Effects::Command::FirmwareUpdate) {
         ArduinoOTA.handle();
         if (millis() > (master->getCommandStart() + 180000)) {
             mqttCtrl.publishInformation("No update started for 180s. Rebooting.");
             ESP.restart();
         }
    } else {

        for (auto& i : lights) {
            i.second.runCurrentCommand();
            // i.second.runCurrentEffect();
        }
        // effects.runCurrentCommand();
        // effects.runCurrentEffect();

        EVERY_N_SECONDS(60) {
            mqttCtrl.publishStatus();
            publishInformationData();
        }

        FastLED.show();
        delay(1000/FPS);
    }
}
