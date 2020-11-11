/*
 * A LED strip controller made to work with ESP32 and Teensy 4.0
 *
 * Currently a farily custom firmware tailored to work with various decoration
 * lights. Mainly for my daughters book shelf and bed lights.
 *
 * - Uses MQTT to talk to a MQTT broker.
 * - Implementes the mqtt_json interface to get commands from a home assistant
 *   server. (https://home-assistant.io)
 * - Uses SPIFFS to store configuration and separate configuration from firmware
 * - Connects to a 3.5mm minijack and uses FFT to create audio responsive light
 * displays.
 *
 * MIT License
 *
 * Copyright (c) 2018-2020 Thomas Malt
 */

#include <Arduino.h>
#include <FastLED.h>

#include <Effects.hpp>
#include <LightState.hpp>

#ifdef IS_ESP32
#include <ArduinoOTA.h>
#endif  // IS_ESP32

#include <EventDispatcher.hpp>
#include <LedshelfConfig.hpp>

// #include <MQTTController.hpp>
// // #include <WiFiController.h>
//
// #ifdef IS_TEENSY
// #include <SerialMQTT.hpp>
// #endif

FASTLED_USING_NAMESPACE

// global objects
CRGBArray<LED_COUNT> leds;

LedshelfConfig config;
Effects::Controller effects;
LightState::Controller lightState;

EventDispatcher hub;

// #ifdef IS_ESP32
// MQTTController mqttCtrl(VERSION, config, lightState, effects);
// #endif
//
// #ifdef IS_TEENSY
// SerialMQTT serialCtrl;
// #endif

uint16_t commandFrames = FPS;
uint16_t commandFrameCount = 0;
ulong commandStart = 0;
uint8_t updateProgress = 0;

// ========================================================================
// FastLED Setup
// ========================================================================
// Function that does the actual fast led setup on start.
// Run by arduino setup function.
void setupFastLED() {
#ifdef DEBUG
  Serial.println("[main] Setting up LED:");
  Serial.printf("[main]   number of leds: %i, max mA: %i\n", LED_COUNT, LED_mA);
#endif

#ifdef LED_CLOCK
#ifdef DEBUG
  Serial.printf("[main]   type: SK9822, data: %i, clock: %i.\n", LED_DATA,
                LED_CLOCK);
#endif
  FastLED
      .addLeds<LED_TYPE, LED_DATA, LED_CLOCK, LED_COLOR_ORDER,
               DATA_RATE_MHZ(12)>(leds, LED_COUNT)
      .setCorrection(TypicalSMD5050);
#else
#ifdef DEBUG
  Serial.printf("[main]   type: WS2812B, data: %i\n", LED_DATA);
#endif
  FastLED.addLeds<LED_TYPE, LED_DATA, LED_COLOR_ORDER>(leds, LED_COUNT)
      .setCorrection(TypicalSMD5050);
#endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_mA);

  LightState::LightState &currentState = lightState.getCurrentState();

#ifdef DEBUG
  Serial.printf("[main] state is: '%s', effect: '%s'\n",
                currentState.state ? "On" : "Off", currentState.effect.c_str());
  FastLED.countFPS(FPS);
#endif

  FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

  effects.setup(leds, LED_COUNT, currentState);

  hub.onStateChange(
      [](LightState::LightState s) { effects.handleStateChange(s); });
}
// END OF setupFastLED

// ========================================================================
// Arduino OTA Setup
// ========================================================================
#ifdef IS_ESP32
uint8_t OTA_HUE = 64;
void setupArduinoOTA() {
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setPassword(config.mqtt_password.c_str());
  ArduinoOTA
      .onStart([]() {
        // U_FLASH or U_SPIFFS
        String type =
            (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
        // using SPIFFS.end()
        hub.publishInformation((String("Start updating " + type)).c_str());
      })
      .onEnd([]() { hub.publishInformation("Finished"); })
      .onProgress([](uint32_t progress, uint32_t total) {
        uint8_t percent = progress / (total / 100);
        uint8_t range = map(percent, 0, 100, 0, 15);
        uint8_t step = 255 / 15;
        EVERY_N_MILLIS(25) {
          OTA_HUE += 1;

          for (int i = 0; i <= range; i++) {
            uint8_t step_hue = OTA_HUE + (i * step);
            leds[i] = CHSV(step_hue, 255, 255);
          }
          // leds(0, range) = CHSV(96, 255, 255);
          FastLED.show();
        }
#ifdef DEBUG
        EVERY_N_MILLIS(1000) {
          Serial.printf("  - Progress: %u %u - percent: %u, range: %u\n",
                        progress, total, percent, range);
        }
#endif
      })
      .onError([](ota_error_t error) {
#ifdef DEBUG
        Serial.printf("Error[%u]: ", error);
#endif
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
        hub.publishInformation(message.c_str());
      });
}
#endif  // IS_ESP32

/* ======================================================================
 * SETUP
 * ======================================================================
 */
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.printf("Starting version %s...\n", VERSION);
#endif

  config.setup();
  lightState.initialize();

  hub.setup();
  hub.setEffects(&effects);
  hub.setLightState(&lightState);

#ifdef IS_ESP32
  setupArduinoOTA();
#endif  // IS_ESP32

  delay(2000);

  setupFastLED();
}

/* ======================================================================
 * LOOP
 * ======================================================================
 */
unsigned long timetowait = 1000 / FPS;
unsigned long previoustime = 0;

void loop() {
  hub.loop();

  if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
#ifdef IS_ESP32
    ArduinoOTA.handle();
    if (millis() > (effects.commandStart + 30000)) {
      hub.publishInformation("No update started for 180s. Rebooting.");
      delay(1000);
      ESP.restart();
    }
#else
#ifdef DEBUG
    Serial.println(
        "Command set to firmware update, but not running on OTA compatible "
        "board.");
#endif  // DEBUG
    effects.setCurrentCommand(Effects::Command::None);
#endif
  } else {
    effects.runCurrentCommand();
    effects.runCurrentEffect();
  }
#ifdef DEBUG
  EVERY_N_SECONDS(10) {
    Serial.printf("FPS: %i heap: %i, size: %i, cpu: %i\n", FastLED.getFPS(),
                  ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getCpuFreqMHz());
  }
#endif

  EVERY_N_MILLIS(timetowait) { FastLED.show(); }
}
