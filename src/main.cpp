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

#include <debug.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <WiFiController.h>
#include <FastLED.h>
#include <Effects.h>
#include <MQTTController.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>

FASTLED_USING_NAMESPACE

static const char *VERSION = "v0.4.0";

// Fastled definitions
static const uint8_t GPIO_DATA = 18;

// 130 bed lights
// 384 shelf lights
static const uint16_t NUM_LEDS = 256;
static const uint8_t FPS = 60;
static const uint8_t FASTLED_SHOW_CORE = 0;

// Using esp32 other core to run FastLED.show().
// -- Task handles for use in the notifications
// static TaskHandle_t FastLEDshowTaskHandle   = 0;
// static TaskHandle_t userTaskHandle          = 0;

// A command to hold the command we're currently executing.

// global objects
CRGBArray<NUM_LEDS> leds;
LedshelfConfig config;           // read from json config file.
LightStateController lightState; // Own object. Responsible for state.
WiFiController wifiCtrl;
Effects effects;
MQTTController mqttCtrl(VERSION, config, wifiCtrl, lightState, effects);

uint16_t commandFrames = FPS;
uint16_t commandFrameCount = 0;
ulong commandStart = 0;
uint8_t updateProgress = 0;

/**
 * show() for ESP32
 *
 * Call this function instead of FastLED.show(). It signals core 0 to issue a show,
 * then waits for a notification that it is done.
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
// void fastLEDshowESP32()
// {
//     if (userTaskHandle == 0) {
//         // -- Store the handle of the current task, so that the show task can
//         //    notify it when it's done
//         userTaskHandle = xTaskGetCurrentTaskHandle();
//
//         // -- Trigger the show task
//         xTaskNotifyGive(FastLEDshowTaskHandle);
//
//         // -- Wait to be notified that it's done
//         const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
//         ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
//         userTaskHandle = 0;
//     }
// }

/**
 * show Task
 * This function runs on core 0 and just waits for requests to call FastLED.show()
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
// void FastLEDshowTask(void *pvParameters)
// {
//     // -- Run forever...
//     for(;;) {
//         // -- Wait for the trigger
//         ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//
//         // -- Do the show (synchronously)
//         // Serial.printf("ESP: Running fastled.show(): %ld\n", millis());
//         FastLED.show();
//
//         // -- Notify the calling task
//         xTaskNotifyGive(userTaskHandle);
//     }
// }

void setupFastLED()
{
    Serial.println("Setting up LED");
    Serial.printf("  - number of leds: %i\n", NUM_LEDS);
    Serial.printf("  - maximum milliamps: %i\n", config.milliamps);

    lightState.initialize();
    LightState &currentState = lightState.getCurrentState();

    Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(UncorrectedColor);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, config.milliamps);
    FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

    effects.setFPS(FPS);
    effects.setLightStateController(&lightState);
    effects.setLeds(leds, NUM_LEDS);
    effects.setCurrentEffect(currentState.effect);
    effects.setStartHue(currentState.color.h);

    // if (currentState.effect == "") {
    //     effects.setCurrentCommand(Effects::Command::Color);
    // }

    // -- Create the FastLED show task
    // xTaskCreatePinnedToCore(
    //     FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
    //     &FastLEDshowTaskHandle, FASTLED_SHOW_CORE
    // );
}

void setupArduinoOTA()
{
    ArduinoOTA.setPort(3232);
    ArduinoOTA.setPassword(config.password.c_str());
    ArduinoOTA
        .onStart([]() {
            // U_FLASH or U_SPIFFS
            String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            mqttCtrl.publishInformation((String("Start updating " + type)).c_str());
        })
        .onEnd([]() {
            mqttCtrl.publishInformation("Finished");
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            String errmsg;
            switch (error)
            {
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
            String message = "Firmware Update Error [" + String(error) + "]: " + errmsg;
            mqttCtrl.publishInformation(message.c_str());
        });
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting version %s...\n", VERSION);

    config.setup();
    wifiCtrl.setup(config);
    mqttCtrl.setup();

    wifiCtrl.connect();

    setupArduinoOTA();

    delay(3000);
    setupFastLED();
}

void loop()
{
    mqttCtrl.checkConnection();

    if (effects.currentCommandType == Effects::Command::FirmwareUpdate)
    {
        ArduinoOTA.handle();
        if (millis() > (effects.commandStart + 180000))
        {
            mqttCtrl.publishInformation("No update started for 180s. Rebooting.");
            ESP.restart();
        }
    }
    else
    {

        effects.runCurrentCommand();
        effects.runCurrentEffect();

        EVERY_N_SECONDS(60)
        {
            mqttCtrl.publishStatus();
            mqttCtrl.publishInformationData();
        }
        // fastLEDshowESP32();
        FastLED.show();
        delay(1000 / FPS);
    }
}
