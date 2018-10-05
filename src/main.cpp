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
#define FASTLED_ALLOW_INTERRUPTS 0

#include <debug.h>
#include <Arduino.h>
#include <WiFiController.h>
#include <MQTTController.h>
#include <FastLED.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>
#include <Effects.h>

FASTLED_USING_NAMESPACE

// Fastled definitions
static const uint8_t GPIO_DATA         = 18;
static const uint8_t NUM_LEDS          = 150;
static const uint8_t FPS               = 60;
static const uint8_t FASTLED_SHOW_CORE = 0;

// Using esp32 other core to run FastLED.show().
// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle   = 0;
static TaskHandle_t userTaskHandle          = 0;

// A command to hold the command we're currently executing.

// global objects
LedshelfConfig        config;         // read from json config file.
LightStateController  lightState;     // Own object. Responsible for state.
WiFiController        wifiCtrl;
MQTTController        mqttCtrl;
CRGBArray<NUM_LEDS>   leds;
Effects               effects;

uint16_t commandFrames     = FPS;
uint16_t commandFrameCount = 0;
ulong    commandStart      = 0;

/**
 * show() for ESP32
 *
 * Call this function instead of FastLED.show(). It signals core 0 to issue a show,
 * then waits for a notification that it is done.
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
void fastLEDshowESP32()
{
    if (userTaskHandle == 0) {
        // -- Store the handle of the current task, so that the show task can
        //    notify it when it's done
        userTaskHandle = xTaskGetCurrentTaskHandle();

        // -- Trigger the show task
        xTaskNotifyGive(FastLEDshowTaskHandle);

        // -- Wait to be notified that it's done
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        userTaskHandle = 0;
    }
}

/**
 * show Task
 * This function runs on core 0 and just waits for requests to call FastLED.show()
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
void FastLEDshowTask(void *pvParameters)
{
    // -- Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // -- Do the show (synchronously)
        // Serial.printf("ESP: Running fastled.show(): %ld\n", millis());
        FastLED.show();

        // -- Notify the calling task
        xTaskNotifyGive(userTaskHandle);
    }
}

void setupFastLED()
{
    Serial.println("Setting up LED");
    lightState.initialize();
    LightState&  currentState    = lightState.getCurrentState();

    Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

    effects.setFPS(FPS);
    effects.setLightStateController(&lightState);
    effects.setLeds(leds, NUM_LEDS);
    effects.setCurrentEffect(currentState.effect);
    effects.setStartHue(currentState.color.h);

    if (currentState.effect == "") {
        effects.setCurrentCommand(Effects::Command::Color);
    }

    // -- Create the FastLED show task
    xTaskCreatePinnedToCore(
        FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
        &FastLEDshowTaskHandle, FASTLED_SHOW_CORE
    );
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting...\n");

    config.setup();
    wifiCtrl.setup(&config);
    mqttCtrl.setup(&wifiCtrl, &lightState, &config, &effects);

    wifiCtrl.connect();


    // all examples I've seen has a startup grace delay.
    // Just cargo-cult copying that practise.
    delay(2000);
    setupFastLED();
}

void loop() {
    mqttCtrl.checkConnection();

    effects.runCurrentCommand();
    effects.runCurrentEffect();

    // fastLEDshowESP32();
    FastLED.show();
    delay(1000/FPS);
}
