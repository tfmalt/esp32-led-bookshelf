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

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <Effects.h>
#include <FastLED.h>
#include <LedshelfConfig.h>
#include <LightStateController.h>
// #include <MD_MSGEQ7.h>
#include <MQTTController.h>
#include <WiFiController.h>
#include <arduinoFFT.h>

#define SAMPLES 1024
#define SAMPLING_FREQUENCY 40000
#define AMPLITUDE 150

// byte peak[] = {0, 0, 0, 0, 0, 0, 0, 0};
double vReal[SAMPLES];
double vImag[SAMPLES];
// unsigned long newTime, oldTime;
// int dominant_value;

FASTLED_USING_NAMESPACE

// global objects
CRGBArray<NUM_LEDS> leds;
LedshelfConfig config;
LightStateController lightState;
Effects effects;
WiFiController wifiCtrl(config);
MQTTController mqttCtrl(VERSION, config, wifiCtrl, lightState, effects);
arduinoFFT FFT = arduinoFFT();

uint16_t commandFrames = FPS;
uint16_t commandFrameCount = 0;
ulong commandStart = 0;
uint8_t updateProgress = 0;

//
// Function that does the actual fast led setup on start. run by arduino setup
// function
void setupFastLED() {
#ifdef DEBUG
  Serial.println("Setting up LED");
  Serial.printf("  - number of leds: %i, max mA: %i\n", NUM_LEDS, LED_mA);
#endif

#ifdef LED_CLOCK
#ifdef DEBUG
  Serial.printf("  - type: SK9822, data: %i, clock: %i.\n", LED_DATA,
                LED_CLOCK);
#endif

  FastLED
      .addLeds<LED_TYPE, LED_DATA, LED_CLOCK, LED_COLOR_ORDER,
               DATA_RATE_MHZ(12)>(leds, NUM_LEDS)
      .setCorrection(TypicalSMD5050);
#else
#ifdef DEBUG
  Serial.printf("  - type: WS2812B, data: %i\n", LED_DATA);
#endif
  FastLED.addLeds<LED_TYPE, LED_DATA, LED_COLOR_ORDER>(leds, NUM_LEDS)
      .setCorrection(TypicalSMD5050);
#endif

  FastLED.setMaxPowerInVoltsAndMilliamps(5, LED_mA);

  lightState.initialize();
  LightState &currentState = lightState.getCurrentState();

#ifdef DEBUG
  Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");
#endif
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

/* ======================================================================
 * SETUP
 */
void setup() {
#ifdef DEBUG
  Serial.begin(115200);
  Serial.printf("Starting version %s...\n", VERSION);
#endif

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

    EVERY_N_SECONDS(300) {
      mqttCtrl.publishStatus();
      mqttCtrl.publishInformationData();
    }

    // for (int i = 0; i < SAMPLES; i++) {
    //   vReal[i] = analogRead(FTT_AUDIO);
    //   vImag[i] = 0;
    //   // Serial.println(vReal[i]);
    // }
    //
    //     // FFT
    //     FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    //     FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    //     FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);

    // for (int i = 2; i < (SAMPLES / 2); i++) {
    //   vReal[i] = constrain(vReal[i], 0, 65535);
    //   vReal[i] = map(vReal[i], 0, 65535, 0, 8);
    //   if (i <= 2) Serial.printf("%i\t", (uint16_t)vReal[i]);
    //   if (i > 2 && i <= 4) Serial.printf("%i\t", (uint16_t)vReal[i]);
    // }
    // Serial.printf("\n");

    // fastLEDshowESP32();
    FastLED.show();
    delay(1000 / FPS);
  }
}
