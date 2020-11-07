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
#include <Effects.h>
#include <FastLED.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>

#ifdef IS_ESP32
#include <ArduinoOTA.h>
#include <MQTTController.h>
#include <WiFiController.h>
#endif

FASTLED_USING_NAMESPACE

// global objects
CRGBArray<LED_COUNT> leds;
LedshelfConfig config;
LightStateController lightState;
Effects effects;

#ifdef IS_ESP32
WiFiController wifiCtrl(config);
MQTTController mqttCtrl(VERSION, config, wifiCtrl, lightState, effects);
#endif

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
  Serial.println("Setting up LED");
  Serial.printf("  - number of leds: %i, max mA: %i\n", LED_COUNT, LED_mA);
#endif

#ifdef LED_CLOCK
#ifdef DEBUG
  Serial.printf("  - type: SK9822, data: %i, clock: %i.\n", LED_DATA,
                LED_CLOCK);
#endif
  FastLED
      .addLeds<LED_TYPE, LED_DATA, LED_CLOCK, LED_COLOR_ORDER,
               DATA_RATE_MHZ(12)>(leds, LED_COUNT)
      .setCorrection(TypicalSMD5050);
#else
#ifdef DEBUG
  Serial.printf("  - type: WS2812B, data: %i\n", LED_DATA);
#endif
  FastLED.addLeds<LED_TYPE, LED_DATA, LED_COLOR_ORDER>(leds, LED_COUNT)
      .setCorrection(TypicalSMD5050);
#endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_mA);

  lightState.initialize();
  LightState &currentState = lightState.getCurrentState();

#ifdef DEBUG
  Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");
  FastLED.countFPS(FPS);
#endif

  FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

  // effects.setFPS(FPS);
  effects.setup();
  effects.setLightStateController(&lightState);
  effects.setLeds(leds, LED_COUNT);
  effects.setCurrentEffect(currentState.effect);
  effects.setStartHue(currentState.color.h);
}
// END OF setupFastLED

// ========================================================================
// Arduino OTA Setup
// ========================================================================
#ifdef IS_ESP32
uint8_t OTA_HUE = 64;
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
        mqttCtrl.publishInformation(message.c_str());
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

#ifdef IS_ESP32
  wifiCtrl.setup();
  wifiCtrl.connect();
  mqttCtrl.setup();

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
#ifdef IS_ESP32
  mqttCtrl.checkConnection();
#endif

  if (effects.currentCommandType == Effects::Command::FirmwareUpdate) {
#ifdef IS_ESP32
    ArduinoOTA.handle();
    if (millis() > (effects.commandStart + 30000)) {
      mqttCtrl.publishInformation("No update started for 180s. Rebooting.");
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
#ifdef IS_ESP32
    EVERY_N_SECONDS(60) { mqttCtrl.publishStatus(); }

#ifdef DEBUG
    EVERY_N_SECONDS(60) {
#else
    EVERY_N_SECONDS(300) {
#endif  // DEBUG
      mqttCtrl.publishInformationData();
    }
#endif  // IS_ESP32
  }
#ifdef DEBUG
  EVERY_N_SECONDS(10) {
    Serial.print("FPS: ");
    Serial.println(FastLED.getFPS());
  }
#endif

  EVERY_N_MILLIS(timetowait) { FastLED.show(); }
}
