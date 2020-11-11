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
#include <LedshelfOTA.hpp>
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

EventDispatcher EventHub;

// global objects
CRGBArray<LED_COUNT> leds;

LedshelfConfig config;
Effects::Controller effects;
LightState::Controller lightState;

// EventDispatcher hub;

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
  FastLED.addLeds<LED_TYPE, LED_DATA, LED_CLOCK, LED_COLOR_ORDER,
                  DATA_RATE_MHZ(12)>(leds, LED_COUNT);
#else
#ifdef DEBUG
  Serial.printf("[main]   type: WS2812B, data: %i\n", LED_DATA);
#endif
  FastLED.addLeds<LED_TYPE, LED_DATA, LED_COLOR_ORDER>(leds, LED_COUNT);
#endif

  FastLED.setCorrection(TypicalSMD5050);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_mA);
  FastLED.setBrightness(0);  // Start at 0 will set state later.

#ifdef DEBUG
  // Serial.printf("[main] state is: '%s', effect: '%s'\n",
  //               currentState.state ? "On" : "Off",
  //               currentState.effect.c_str());
  FastLED.countFPS(FPS);
#endif
}
// END OF setupFastLED

/* ======================================================================
 * SETUP
 * ======================================================================
 */
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[main] Starting version %s...\n", VERSION);
#endif

  lightState.initialize();

  EventHub.setup();
  EventHub.setEffects(&effects);
  EventHub.setLightState(&lightState);
  EventHub.onStateChange(
      [](LightState::LightState s) { effects.handleStateChange(s); });

#ifdef IS_ESP32
  LedshelfOTA::setup(leds);
  // setupArduinoOTA();
  EventHub.onFirmwareUpdate([]() {
    LedshelfOTA::start();

    effects.setCurrentEffect(Effects::Effect::NullEffect);
    effects.setCurrentCommand(Effects::Command::FirmwareUpdate);
    effects.runCurrentCommand();
  });
#endif  // IS_ESP32

  delay(2000);

  setupFastLED();

  // FastLED.setBrightness(currentState.state ? currentState.brightness : 0);
  // LightState::LightState &currentState = lightState.getCurrentState();
  effects.setup(leds, LED_COUNT, lightState.getCurrentState());
}

/* ======================================================================
 * LOOP
 * ======================================================================
 */
unsigned long timetowait = 1000 / FPS;
unsigned long previoustime = 0;

void loop() {
  EventHub.loop();

  if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
#ifdef IS_ESP32
    LedshelfOTA::handle();
    if (millis() > (effects.commandStart + 30000)) {
      EventHub.publishInformation("No update started for 180s. Rebooting.");
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
