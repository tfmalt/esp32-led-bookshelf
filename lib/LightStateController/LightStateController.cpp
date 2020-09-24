/**
 * Class responsible for keeping track of the state of the light.
 *
 * Copyright 2018 Thomas Malt <thomas@malt.no>
 */
#include "LightStateController.h"
#include <debug.h>
#include <SPIFFS.h>
#include <FS.h>
#include <ArduinoJson.h>

LightStateController::LightStateController()
{
    Color defaultColor = {0};

    LightStatus defaultStatus = {false};
    defaultStatus.status = 255;

    defaultState.color = defaultColor;
    defaultState.effect = "colorloop";
    defaultState.brightness = 255;
    defaultState.transition = 1;
    defaultState.state = false;
    defaultState.status = defaultStatus;
}

LightStateController &LightStateController::setCurrentState(const char *stateString)
{
    Serial.println("DEBUG: LightStateController up and running.");
    Serial.printf("  - %s\n", stateFile);
    Serial.printf("  - %i\n", currentState.color_temp);
    return *this;
}

uint8_t LightStateController::initialize()
{
    currentState = defaultState;
    Serial.printf("DEBUG: Reading in '%s'\n", stateFile);
    File file = SPIFFS.open(stateFile, "r");
    if (!file)
        return LIGHT_STATEFILE_NOT_FOUND;

    Serial.println("  - Opened file ok.");

    StaticJsonDocument<512> state;
    auto error = deserializeJson(state, file);

    if (error)
    {
        Serial.println("deserializejson of light state failed with error:");
        Serial.println(error.c_str());
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
    return 0;
}

LightState &LightStateController::parseNewState(byte *payload)
{
    try
    {
        LightState newState = getLightStateFromPayload(payload);
#ifdef DEBUG
        printStateDebug(newState);
#endif
        currentState = newState;
    }
    catch (LightState errState)
    {
        String error = "Unknown";
        if (errState.status.status == LIGHT_MQTT_JSON_FAILED)
        {
            error = "Parsing of json failed.";
        }
        else if (errState.status.status == LIGHT_MQTT_JSON_NO_STATE)
        {
            error = "Did not get correct state parameter in Json.";
        }

        Serial.printf("ERROR: parsing of new state threw error: %s\n", error.c_str());
    }

    Serial.println("  - Saving current state to file.");
    saveCurrentState();

    return currentState;
}

void LightStateController::printStateDebug(LightState &state)
{
#ifdef DEBUG
    Serial.println("DEBUG: got new LightState:");
    Serial.printf(
        "  - has state: %s, value: %s\n",
        (state.status.hasState ? "true" : "false"),
        (state.state) ? "On" : "Off");
    Serial.printf(
        "  - has brightness: %s, value: %i\n",
        (state.status.hasBrightness ? "true" : "false"),
        state.brightness);
    Serial.printf(
        "  - has color_temp: %s, value: %i\n",
        (state.status.hasColorTemp ? "true" : "false"),
        state.color_temp);
    Serial.printf(
        "  - has transition: %s, value: %i\n",
        (state.status.hasTransition ? "true" : "false"),
        state.transition);
    Serial.printf(
        "  - has effect: %s, value: '%s'\n",
        (state.status.hasEffect ? "true" : "false"),
        state.effect.c_str());
    Serial.printf("  - has color: %s, value: [%i,%i,%i,%0.2f,%0.2f]\n",
                  (state.status.hasColor ? "true" : "false"),
                  state.color.r, state.color.g, state.color.b,
                  state.color.h, state.color.s);
#endif
}

/**
 * Takes a JsonObject reference and returns a color struct.
 */
// Color LightStateController::getColorFromJsonObject(JsonObject &obj)
// {
//     Color color = {0};
//
//     //if (!obj.containsKey("color"))
//     //    return color;
//     //
//     //JsonObject cJson = root["color"].as<JsonObject>();
//
//     // if (!cJson.success())
//     //     return color;
//
//     color.r = (uint8_t)obj["r"];
//     color.g = (uint8_t)obj["g"];
//     color.b = (uint8_t)obj["b"];
//     color.h = (float)obj["h"];
//     color.s = (float)obj["s"];
//
//     return color;
// }

LightState LightStateController::getLightStateFromPayload(byte *payload)
{
    StaticJsonDocument<256> data;
    auto error = deserializeJson(data, payload);

    LightState newState = currentState;

    newState.status = {false};

    if (error)
    {
        newState.status.status = LIGHT_MQTT_JSON_FAILED;
        newState.status.success = false;
        throw newState;
    }

#ifdef DEBUG
    // char output[256] = "";
    Serial.println("DEBUG: JSON:");
    serializeJson(data, Serial);
#endif

    if (!data.containsKey("state"))
    {
        newState.status.status = LIGHT_MQTT_JSON_NO_STATE;
        newState.status.success = false;
        throw newState;
    }

    newState.state = data["state"] == "ON" ? true : false;
    newState.status.hasState = true;

    if (data.containsKey("brightness"))
    {
        newState.status.hasBrightness = true;
        newState.brightness = data["brightness"];
    }

    if (data.containsKey("white_value"))
    {
        newState.status.hasWhiteValue = true;
        newState.white_value = data["white_value"];
    }

    if (data.containsKey("color_temp"))
    {
        newState.status.hasColorTemp = true;
        newState.color_temp = data["color_temp"];
    }

    if (data.containsKey("transition"))
    {
        newState.status.hasTransition = true;
        newState.transition = data["transition"];
    }

    if (data.containsKey("effect"))
    {
        newState.status.hasEffect = true;
        newState.effect = data["effect"].as<String>();

        if (newState.effect == "none")
        {
            newState.effect = "";
        }
    }

    if (data.containsKey("color") && data["color"].is<JsonObject>())
    {
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

void LightStateController::serializeCurrentState(char *output, int length)
{
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

// JsonObject &LightStateController::currentStateAsJson(JsonObject &object, JsonObject &color)
// {
//     LightState state = getCurrentState();
//
//     color["r"] = state.color.r;
//     color["g"] = state.color.g;
//     color["b"] = state.color.b;
//     color["h"] = state.color.h;
//     color["s"] = state.color.s;
//
//     object["state"] = (state.state) ? "ON" : "OFF";
//     object["brightness"] = state.brightness;
//     object["color_temp"] = state.color_temp;
//     object["color"] = color;
//     object["effect"] = state.effect;
//
//     return object;
// }

uint8_t LightStateController::saveCurrentState()
{
    // StaticJsonDocument<256> doc;
    // JsonObject &object = jsonBuffer.createObject();
    // JsonObject &color = jsonBuffer.createObject();
    // JsonObject &current = createCurrentStateJsonObject(object, color);

    char json[256];
    serializeCurrentState(json, 256);

    File file = SPIFFS.open(stateFile, "w+");

    file.println(json);
    file.close();
    return LIGHT_STATEFILE_WROTE_SUCCESS;
}

LightState &LightStateController::getCurrentState()
{
    return currentState;
}
