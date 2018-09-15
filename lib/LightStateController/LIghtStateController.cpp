#include "LightStateController.h"
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

LightStateController::LightStateController() {
    Serial.println("DEBUG: Light state controller debug.");
    parseStateFile();
}

LightStateController::LightStateController(const char* stateString) {

}

bool LightStateController::setCurrentState(const char* stateString) {
    Serial.println("DEBUG: LightStateController up and running.");
    Serial.printf("  - %s\n", stateFile);
    Serial.printf("  - %i\n", currentState.color_temp);
    return true;
}

uint8_t LightStateController::parseStateFile() {
    File file = SPIFFS.open(stateFile, "r");
    if (!file) return LIGHT_STATEFILE_NOT_FOUND;

    StaticJsonBuffer<256> buffer;
    JsonObject& root = buffer.parseObject(file);

    if (!root.success()) return LIGHT_STATEFILE_JSON_FAILED;

    file.close();

    currentState.state = root["state"] || false; //  == "ON") ? true : false;

    // if (root.containsKey("effect"))
    currentState.effect     = (const char*) root["effect"];
    currentState.brightness = root["brightness"] || 255;
    currentState.color_temp = root["color_temp"] || 200;
    currentState.transition = root["transition"] || 1;
    
    return 0;
}

uint8_t LightStateController::newState(byte* payload) {
    StaticJsonBuffer<256>   jsonBuffer;
    JsonObject&             root     = jsonBuffer.parseObject(payload);
    LightState              newState = {0};

    if (!root.success())            return LIGHT_MQTT_JSON_FAILED;
    if (!root.containsKey("state")) return LIGHT_MQTT_JSON_NO_STATE;

}