
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>

#include <EventDispatcher.hpp>
#include <LedshelfConfig.hpp>
#include <functional>
#include <string>

namespace LedshelfOTA {

typedef std::function<void()> OTAHandlerFunction;

LedshelfConfig config;
CRGB* leds;

uint8_t OTA_HUE = 64;

// ========================================================================
// Arduino OTA Setup
// ========================================================================
void setup(CRGB* l) {
#ifdef IS_ESP32

  leds = l;
  ArduinoOTA.setPort(3232);
  ArduinoOTA.setPassword(config.mqtt_password.c_str());

  // ======================================================================
  // Arduino OTA onStart Handler
  // ======================================================================
  ArduinoOTA.onStart([]() {
    std::string type =
        (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS
    // using SPIFFS.end()

    std::string msg = "Starting firmware update of: " + type;

#ifdef DEBUG
    Serial.printf("[ota] %s\n", msg.c_str());
#endif

    EventHub.publishInformation(msg);
  });

  // ======================================================================
  // Arduino OTA onEnd Handler
  // ======================================================================
  ArduinoOTA.onEnd([]() {
#ifdef DEBUG
    Serial.println("[ota] finished firmware update.");
#endif
    EventHub.publishInformation("Finished firmware update");
    FastLED.setBrightness(0);
    FastLED.show();
  });

  // ======================================================================
  // Arduino OTA onProgress Handler
  // ======================================================================
  ArduinoOTA.onProgress([](uint32_t progress, uint32_t total) {
    uint8_t percent = progress / (total / 100);
    uint8_t range = map(percent, 0, 100, 0, 15);
    uint8_t step = 255 / 15;
    EVERY_N_MILLIS(25) {
      OTA_HUE += 1;

      for (int i = 0; i <= range; i++) {
        uint8_t step_hue = OTA_HUE + (i * step);
        leds[i] = CHSV(step_hue, 255, 255);
      }

      FastLED.show();
    }

#ifdef DEBUG
    EVERY_N_MILLIS(1000) {
      Serial.printf("[ota] progress: %u %u - percent: %u, range: %u\n",
                    progress, total, percent, range);
    }
#endif
  });

  // ======================================================================
  // Arduino OTA onError handler
  // ======================================================================
  ArduinoOTA.onError([](ota_error_t error) {
#ifdef DEBUG
    Serial.printf("[ota] Error[%u]: ", error);
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
    String message = "Firmware Update Error [" + String(error) + "]: " + errmsg;
    EventHub.publishInformation(message.c_str());
  });
}

void start() {
  Serial.println("[ota] received update notification. beginning.");
  ArduinoOTA.begin();
}

void handle() { ArduinoOTA.handle(); }

#endif  // IS_ESP32

}  // namespace LedshelfOTA