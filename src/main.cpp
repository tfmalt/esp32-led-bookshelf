/*
 * A first attempt at writing code for ESP32.
 * This is going to be a led light controller for lights on my daughters book
 * shelf.
 *
 * - Uses MQTT over TLS to talk to a MQTT broker.
 * - Implementes the mqtt_json interface to get commands from a home assistant
 *   server. (https://home-assistant.io)
 * - Uses SPIFFS to store configuration and separate configuration from firmware
 *
 * MIT License
 *
 * Copyright (c) 2018-2020 Thomas Malt
 */

// #include <debug.h>
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Effects.h>
#include <FastLED.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>
#include <MQTTController.h>
#include <WiFiController.h>

FASTLED_USING_NAMESPACE

// Fastled definitions
// static const uint8_t GPIO_DATA = 18;

// 130 bed lights
// 384 shelf lights
// static const uint16_t NUM_LEDS = 128;
// static const uint8_t FPS = 60;
// static const uint8_t FASTLED_SHOW_CORE = 0;

// global objects
CRGBArray<NUM_LEDS> leds;

LedshelfConfig config;            // read from json config file.
LightStateController lightState;  // Own object. Responsible for state.
Effects effects;
WiFiController wifiCtrl(config);
MQTTController mqttCtrl(VERSION, config, wifiCtrl, lightState, effects);

uint16_t commandFrames = FPS;
uint16_t commandFrameCount = 0;
ulong commandStart = 0;
uint8_t updateProgress = 0;

//
// Function that does the actual fast led setup on start. run by arduino setup
// function
void setupFastLED() {
  Serial.println("Setting up LED");
  Serial.printf("  - number of leds: %i\n", NUM_LEDS);
  Serial.printf("  - maximum milliamps: %i\n", config.fastled_milliamps);

#ifdef GPIO_CLOCK
  Serial.printf("  - type: SK9822, data: %i, clock: %i.\n", GPIO_DATA,
                GPIO_CLOCK);
  FastLED
      .addLeds<LED_TYPE, GPIO_DATA, GPIO_CLOCK, LED_COLOR_ORDER,
               DATA_RATE_MHZ(12)>(leds, NUM_LEDS)
      .setCorrection(TypicalSMD5050);
#else
  Serial.printf("  - type: WS2812B, data: %i\n", GPIO_DATA);
  FastLED.addLeds<LED_TYPE, GPIO_DATA, LED_COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalSMD5050);
#endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5, config.fastled_milliamps);

  lightState.initialize();
  LightState &currentState = lightState.getCurrentState();

  Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");
  FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

  // effects.setFPS(FPS);
  effects.setLightStateController(&lightState);
  effects.setLeds(leds, NUM_LEDS);
  effects.setCurrentEffect(currentState.effect);
  effects.setStartHue(currentState.color.h);
}

void setupArduinoOTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setPassword(config.mqtt_password);
  ArduinoOTA
      .onStart([]() {
        // U_FLASH or U_SPIFFS
        String type =
            (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        mqttCtrl.publishInformation((String("Start updating " + type)).c_str());
      })
      .onEnd([]() { mqttCtrl.publishInformation("Finished"); })
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
        String message =
            "Firmware Update Error [" + String(error) + "]: " + errmsg;
        mqttCtrl.publishInformation(message.c_str());
      });
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting version %s...\n", VERSION);

  config.setup();
  wifiCtrl.setup();
  wifiCtrl.connect();
  mqttCtrl.setup();

  setupArduinoOTA();

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
      mqttCtrl.publishInformationData();
    }
    // fastLEDshowESP32();
    FastLED.show();
    delay(1000 / FPS);
  }
}
