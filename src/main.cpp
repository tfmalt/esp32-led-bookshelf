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

#ifdef ESP32
// Over the air update is only available on esp.
#include <LedshelfOTA.hpp>
#endif  // ESP32
#ifdef TEENSY
#include <TeensyUtil.hpp>
#endif
#include <EventDispatcher.hpp>
#include <LedshelfConfig.hpp>

FASTLED_USING_NAMESPACE

// EventDispatcher EventHub;
CRGBArray<LED_COUNT> leds;
LedshelfConfig config;
Effects::Controller effects;
LightState::Controller lightState;

uint16_t commandFrames = FPS;
uint16_t commandFrameCount = 0;
ulong commandStart = 0;
uint8_t updateProgress = 0;

// ========================================================================
// FastLED Setup
// ========================================================================
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
  delay(5000);
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println();
  Serial.printf("[main] Starting version %s...\n", VERSION);

  EventHub.enableVerboseOutput(true);
#endif

  lightState.initialize();

  EventHub.setup();
  // EventHub.setEffects(&effects);
  EventHub.setLightState(&lightState);
  EventHub.onStateChange(
      [](LightState::LightState s) { effects.handleStateChange(s); });

#ifdef ESP32
  LedshelfOTA::setup(leds);
  // setupArduinoOTA();
  EventHub.onFirmwareUpdate([]() {
    LedshelfOTA::start();

    effects.setCurrentEffect(Effects::Effect::NullEffect);
    effects.setCurrentCommand(Effects::Command::FirmwareUpdate);
    effects.runCurrentCommand();
  });
#endif  // ESP32

  delay(2000);

  setupFastLED();

  effects.setup(leds, LED_COUNT, lightState.getCurrentState());
  // EventHub.handleReady();
}

/* ======================================================================
 * LOOP
 * ======================================================================
 */
unsigned long timetowait = 1000 / FPS;
unsigned long previoustime = 0;

void loop() {
  EventHub.loop();

#ifdef TEENSY
  if (EventHub.mqtt.getHeartbeatAge() > 300000) {
    // reboot if heartbeat is older than 300s = 5min.
    Serial.println("[main] !!! heartbeat is older than 5 minutes. reboot.");
    Util::restart();
  }
#endif

  if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
#ifdef ESP32
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
#ifdef ESP32
    Serial.printf("[main] ||| fps: %i heap: %i, size: %i, cpu: %i\n",
                  FastLED.getFPS(), ESP.getFreeHeap(), ESP.getHeapSize(),
                  ESP.getCpuFreqMHz());
#endif
#ifdef TEENSY
    Serial.printf("[main] ||| fps: %i, uptime: %lus, heartbeat age: %lu\n",
                  FastLED.getFPS(), (uint32_t)(millis() / 1000),
                  EventHub.mqtt.getHeartbeatAge());

#endif
  }
#endif  // DEBUG

  EVERY_N_MILLIS(timetowait) { FastLED.show(); }
}
