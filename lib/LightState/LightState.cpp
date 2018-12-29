/**
 * Class responsible for keeping track of the state of the light.
 *
 * Copyright 2018 Thomas Malt <thomas@malt.no>
 */
#include "LightState.h"
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

LightState::LightState()
{
    Color defaultColor = {0};

    LightStatus defaultStatus = {false};
    defaultStatus.status = 255;

    defaultState.color      = defaultColor;
    defaultState.effect     = "colorloop";
    defaultState.brightness = 128;
    defaultState.transition = 1;
    defaultState.state      = false;
    defaultState.status     = defaultStatus;
}

uint8_t LightState::initialize(const char* stateFile)
{
    currentState = defaultState;
    Serial.printf("LightState: Reading in '%s'\n", stateFile);
    File file = SPIFFS.open(stateFile, "r");
    if (!file) return LIGHT_STATEFILE_NOT_FOUND;

    Serial.println("  - Opened file ok.");

    StaticJsonBuffer<512> buffer;
    JsonObject& root = buffer.parseObject(file);

    if (!root.success()) return LIGHT_STATEFILE_JSON_FAILED;

    Serial.println("  - Parsed JSON ok.");

    currentState.state = (root["state"] == "ON") ? true : false;

    // if (root.containsKey("effect"))
    currentState.effect     = root["effect"].as<String>();
    currentState.brightness = (uint8_t) root["brightness"];
    currentState.color_temp = (uint16_t) root["color_temp"];
    currentState.color.r    = (uint8_t) root["color"]["r"];
    currentState.color.g    = (uint8_t) root["color"]["g"];
    currentState.color.b    = (uint8_t) root["color"]["b"];
    currentState.color.h    = (float) root["color"]["h"];
    currentState.color.s    = (float) root["color"]["s"];

    file.close();
    return 0;
}

LightStateData& LightState::parseNewState(byte* payload)
{
    try {
        LightStateData newState = getLightStateFromPayload(payload);
        #ifdef DEBUG
        printStateDebug(newState);
        #endif
        currentState = newState;
    }
    catch (LightStateData errState) {
        String error = "Unknown";
        if (errState.status.status == LIGHT_MQTT_JSON_FAILED) {
            error = "Parsing of json failed.";
        }
        else if (errState.status.status == LIGHT_MQTT_JSON_NO_STATE) {
            error = "Did not get correct state parameter in Json.";
        }

        Serial.printf("ERROR: parsing of new state threw error: %s\n", error.c_str());
    }

    Serial.println("  - Saving current state to file.");
    saveCurrentState();

    return currentState;
}

/**
 * Takes a JsonObject reference and returns a color struct.
 */
Color LightState::getColorFromJsonObject(JsonObject& root)
{
    Color color = {0};

    if (!root.containsKey("color")) return color;

    JsonObject& cJson = root["color"].as<JsonObject>();

    if (!cJson.success()) return color;

    color.r = (uint8_t) cJson["r"];
    color.g = (uint8_t) cJson["g"];
    color.b = (uint8_t) cJson["b"];
    color.h = (float) cJson["h"];
    color.s = (float) cJson["s"];

    return color;
}

LightStateData LightState::getLightStateFromPayload(byte* payload)
{
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject&           root     = jsonBuffer.parseObject(payload);
    LightStateData        newState = currentState;

    newState.status = {false};

    if (!root.success()) {
        newState.status.status = LIGHT_MQTT_JSON_FAILED;
        newState.status.success = false;
        throw newState;
    }

    char output[256] = "";
    root.printTo(output, 256);

    #ifdef DEBUG
    Serial.printf("DEBUG: JSON: '%s'\n", output);
    #endif

    if (!root.containsKey("state")) {
        newState.status.status = LIGHT_MQTT_JSON_NO_STATE;
        newState.status.success = false;
        throw newState;
    }

    newState.state = root["state"] == "ON" ? true : false;
    newState.status.hasState = true;

    if (root.containsKey("brightness")) {
        newState.status.hasBrightness = true;
        newState.brightness = root["brightness"];
    }

    if (root.containsKey("white_value")) {
        newState.status.hasWhiteValue = true;
        newState.white_value = root["white_value"];
    }

    if (root.containsKey("color_temp")) {
        newState.status.hasColorTemp = true;
        newState.color_temp = root["color_temp"];
    }

    if (root.containsKey("transition")) {
        newState.status.hasTransition = true;
        newState.transition = root["transition"];
    }

    if (root.containsKey("effect")) {
        newState.status.hasEffect = true;
        newState.effect = root["effect"].as<String>();

        if (newState.effect == "none") {
            newState.effect = "";
        }
    }

    if (root.containsKey("color")) {
        newState.status.hasColor = true;
        newState.color = getColorFromJsonObject(root);
    }

    newState.status.success = true;
    return newState;
}

void LightState::printStateJsonTo(char* output)
{
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& object  = jsonBuffer.createObject();
    JsonObject& color   = jsonBuffer.createObject();
    JsonObject& current = createCurrentStateJsonObject(object, color);

    current.printTo(output, 256);
}

JsonObject& LightState::createCurrentStateJsonObject(JsonObject& object, JsonObject& color)
{
    LightStateData state = getCurrentState();

    color["r"] = state.color.r;
    color["g"] = state.color.g;
    color["b"] = state.color.b;
    color["h"] = state.color.h;
    color["s"] = state.color.s;

    object["state"]       = (state.state) ? "ON" : "OFF";
    object["brightness"]  = state.brightness;
    object["color_temp"]  = state.color_temp;
    object["color"]       = color;
    object["effect"]      = state.effect;

    return object;
}

uint8_t LightState::saveCurrentState()
{
    StaticJsonBuffer<256> jsonBuffer;
    JsonObject& object = jsonBuffer.createObject();
    JsonObject& color = jsonBuffer.createObject();
    JsonObject& current = createCurrentStateJsonObject(object, color);

    File file = SPIFFS.open(stateFile, "w+");
    current.prettyPrintTo(file);

    file.close();
    return LIGHT_STATEFILE_WROTE_SUCCESS;
}


LightStateData& LightState::getCurrentState()
{
    return currentState;
}

void LightState::printStateDebug(LightState& state)
{
    #ifdef DEBUG
    Serial.println("DEBUG: got new LightState:");
    Serial.printf(
        "  - has state: %s, value: %s\n",
        (state.status.hasState ? "true" : "false"),
        (state.state) ? "On": "Off"
    );
    Serial.printf(
        "  - has brightness: %s, value: %i\n",
        (state.status.hasBrightness ? "true" : "false"),
        state.brightness
    );
    Serial.printf(
        "  - has color_temp: %s, value: %i\n",
        (state.status.hasColorTemp ? "true" : "false"),
        state.color_temp
    );
    Serial.printf(
        "  - has transition: %s, value: %i\n",
        (state.status.hasTransition ? "true" : "false"),
        state.transition
    );
    Serial.printf(
        "  - has effect: %s, value: '%s'\n",
        (state.status.hasEffect ? "true" : "false"),
        state.effect.c_str()
    );
    Serial.printf("  - has color: %s, value: [%i,%i,%i,%0.2f,%0.2f]\n",
        (state.status.hasColor ? "true" : "false"),
        state.color.r, state.color.g, state.color.b,
        state.color.h, state.color.s
    );
    #endif
}
