#include "LedShelf.h"

Color getColorFromJson(JsonObject& root) {
    Color color = {0};

    if (!root.containsKey("color")) {
        Serial.println("DEBUG: Message does not have color.");
        return color;
    }
        
    JsonObject& cJson = root["color"].as<JsonObject>();

    if (!cJson.success()) {
        Serial.println("ERROR: invalid color statement found. Skipping");
        return color;
    }

    color.r = (uint8_t) cJson["r"];
    color.g = (uint8_t) cJson["g"];
    color.b = (uint8_t) cJson["b"]; 
    color.h = (float) cJson["h"]; 
    color.s = (float) cJson["s"]; 

    return color; 
}

LightState getLightStateFromMQTT(byte* message) {
    LightState            state = {0};
    StaticJsonBuffer<420> jsonBuffer;
    JsonObject&           root = jsonBuffer.parseObject(message);

    if (!root.success()) {
       Serial.println("ERROR: parseobject failed.");
       digitalWrite(BUILTIN_LED, LOW);
       return state;
    }

    String json = "";
    root.printTo(json);
    Serial.printf("INFO: payload: %s\n", json.c_str());

    if (!root.containsKey("state")) {
        Serial.println("ERROR: Did not get state in command. ignoring.");
        digitalWrite(BUILTIN_LED, LOW);
        return state;
    }

    // State
    state.state = (root["state"] == "ON") ? true : false;
    Serial.printf("  - DEBUG: Got valid state: %i\n", state.state);

    // Effect
    if (root.containsKey("effect")) {
        state.effect = (const char*) root["effect"];
        Serial.printf("  - DEBUG: Got effect: '%s'\n", state.effect.c_str());
    }

    // Color
    state.color = getColorFromJson(root);
    Serial.printf(
        "  - DEBUG: Got color: R:%i, G:%i, B:%i,   H:%0.2f, S:%0.2f\n", 
        state.color.r, state.color.g, state.color.b, 
        state.color.h, state.color.s
    );
    
    // Brightness
    state.brightness = root["brightness"];
    Serial.printf("  - DEBUG: Got brightness: %i\n", state.brightness);

    // Transition
    if (root.containsKey("transition")) {
        state.transition = (uint16_t) root["transition"];
        Serial.printf("  - DEBUG: Got transition: %i\n", state.transition);
    }

    return state;
}

String createJsonString(LightState& state) {
    StaticJsonBuffer<420> jsonBuffer;
    JsonObject& object = jsonBuffer.createObject();
    JsonObject& color = jsonBuffer.createObject();

    color["r"] = state.color.r;
    color["g"] = state.color.g;
    color["b"] = state.color.b;
    color["h"] = state.color.h;
    color["s"] = state.color.s;

    object["state"]       = (state.state) ? "ON" : "OFF"; 
    object["brightness"]  = state.brightness;
    object["color"]       = color;
    object["effect"]      = state.effect.c_str();

    String output = "";
    object.printTo(output);
    return output;
}