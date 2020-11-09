/**
 * Class responsible for keeping track of the state of the light.
 *
 * Copyright 2018-2020 Thomas Malt <thomas@malt.no>
 */
#include "LightStateController.h"

#include <ArduinoJson.h>

#ifdef IS_ESP32
#include <FS.h>
#include <SPIFFS.h>
#endif

LightStateController &LightStateController::setCurrentState(
    const char *stateString) {
#ifdef DEBUG
  Serial.println("DEBUG: LightStateController up and running.");
  Serial.printf("  - %s\n", stateFile);
  Serial.printf("  - %i\n", currentState.color_temp);
#endif
  return *this;
}

uint8_t LightStateController::initialize() {
  currentState = defaultState;
#ifdef IS_ESP32
  if (!SPIFFS.begin()) {
    Serial.println("ERROR: Could not mount spiffs");
    delay(2000);
    ESP.restart();
  }

#ifdef DEBUG
  Serial.printf("  - Light State: Reading in '%s'\n", stateFile);
#endif
  File file = SPIFFS.open(stateFile, "r");
  if (!file) return LIGHT_STATEFILE_NOT_FOUND;
#ifdef DEBUG
  Serial.println("    - Opened file ok.");
#endif

  StaticJsonDocument<512> state;
  auto error = deserializeJson(state, file);

  if (error) {
#ifdef DEBUG
    Serial.println("ERROR: deserializejson of light state failed with error:");
    Serial.println(error.c_str());
#endif
    return LIGHT_STATEFILE_JSON_FAILED;
  }

  currentState.state = (state["state"] == "ON") ? true : false;

  // if (root.containsKey("effect"))
  currentState.effect = state["effect"].as<String>();
  currentState.brightness = (uint8_t)state["brightness"];
  currentState.color_temp = (uint16_t)state["color_temp"];
  currentState.color.r = (uint8_t)state["color"]["r"];
  currentState.color.g = (uint8_t)state["color"]["g"];
  currentState.color.b = (uint8_t)state["color"]["b"];
  currentState.color.h = (float)state["color"]["h"];
  currentState.color.s = (float)state["color"]["s"];

  file.close();
#endif  // IS_ESP32
  return 0;
}

LightState &LightStateController::parseNewState(byte *payload) {
  LightState newState = getLightStateFromPayload(payload);
  currentState = newState;

#ifdef DEBUG
  Serial.println("  - Saving current state to file.");
#endif

  saveCurrentState();

  return currentState;
}

void LightStateController::printStateDebug(LightState &state) {
#ifdef DEBUG
  Serial.println("DEBUG: got new LightState:");
  Serial.printf("  - has state: %s, value: %s\n",
                (state.status.hasState ? "true" : "false"),
                (state.state) ? "On" : "Off");
  Serial.printf("  - has brightness: %s, value: %i\n",
                (state.status.hasBrightness ? "true" : "false"),
                state.brightness);
  Serial.printf("  - has color_temp: %s, value: %i\n",
                (state.status.hasColorTemp ? "true" : "false"),
                state.color_temp);
  Serial.printf("  - has transition: %s, value: %i\n",
                (state.status.hasTransition ? "true" : "false"),
                state.transition);
  Serial.printf("  - has effect: %s, value: '%s'\n",
                (state.status.hasEffect ? "true" : "false"),
                state.effect.c_str());
  Serial.printf("  - has color: %s, value: [%i,%i,%i,%0.2f,%0.2f]\n",
                (state.status.hasColor ? "true" : "false"), state.color.r,
                state.color.g, state.color.b, state.color.h, state.color.s);
#endif
}

LightState LightStateController::getLightStateFromPayload(byte *payload) {
  StaticJsonDocument<256> data;
  auto error = deserializeJson(data, payload);

  LightState newState = currentState;

  newState.status = {false};

  if (error) {
    newState.status.status = LIGHT_MQTT_JSON_FAILED;
    newState.status.success = false;
    // throw newState;
    Serial.printf("ERROR: Got error reading state from payload.\n");
  }

  if (!data.containsKey("state")) {
    newState.status.status = LIGHT_MQTT_JSON_NO_STATE;
    newState.status.success = false;
    // throw newState;
    Serial.println("ERROR: There was no state attribute in json");
  }

  newState.state = data["state"] == "ON" ? true : false;
  newState.status.hasState = true;

  if (data.containsKey("brightness")) {
    newState.status.hasBrightness = true;
    newState.brightness = data["brightness"];
  }

  if (data.containsKey("white_value")) {
    newState.status.hasWhiteValue = true;
    newState.white_value = data["white_value"];
  }

  if (data.containsKey("color_temp")) {
    newState.status.hasColorTemp = true;
    newState.color_temp = data["color_temp"];
  }

  if (data.containsKey("transition")) {
    newState.status.hasTransition = true;
    newState.transition = data["transition"];
  }

  if (data.containsKey("effect")) {
    newState.status.hasEffect = true;
    newState.effect = data["effect"].as<String>();

    if (newState.effect == "none") {
      newState.effect = "";
    }
  }

  if (data.containsKey("color") && data["color"].is<JsonObject>()) {
    newState.status.hasColor = true;
    newState.color = {0};

    auto col = data["color"].as<JsonObject>();

    newState.color.r = (uint8_t)col["r"];
    newState.color.g = (uint8_t)col["g"];
    newState.color.b = (uint8_t)col["b"];
    newState.color.h = (float)col["h"];
    newState.color.s = (float)col["s"];
  }

  newState.status.success = true;
  return newState;
}

/**
 * Serializes the current state into the output buffer
 */
void LightStateController::serializeCurrentState(char *output, int length) {
  LightState state = getCurrentState();
  StaticJsonDocument<256> doc;

  doc["state"] = (state.state) ? "ON" : "OFF";
  doc["brightness"] = state.brightness;
  doc["color_temp"] = state.color_temp;
  doc["effect"] = state.effect;

  JsonObject color = doc.createNestedObject("color");
  color["r"] = state.color.r;
  color["g"] = state.color.g;
  color["b"] = state.color.b;
  color["h"] = state.color.h;
  color["s"] = state.color.s;

  serializeJson(doc, output, length);
}

/**
 * Saves the current state to spiffs or eeprom
 */
uint8_t LightStateController::saveCurrentState() {
#ifdef IS_ESP32
  char json[256];
  serializeCurrentState(json, 256);

  File file = SPIFFS.open(stateFile, "w+");

  file.println(json);
  file.close();
#endif

  return LIGHT_STATEFILE_WROTE_SUCCESS;
}

/**
 * Returns current state
 */
LightState &LightStateController::getCurrentState() { return currentState; }
