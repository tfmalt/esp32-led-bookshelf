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
    color.x = (float) cJson["x"]; 
    color.y = (float) cJson["y"]; 
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
        "  - DEBUG: Got color: R:%i, G:%i, B:%i,   X:%0.2f, Y:%0.2f,   H:%0.2f, S:%0.2f\n", 
        state.color.r, state.color.g, state.color.b, state.color.x, 
        state.color.y, state.color.h, state.color.s
    );
    
    // Brightness
    state.brightness = root["brightness"];
    Serial.printf("  - DEBUG: Got brightness: %i\n", state.brightness);

    // Color temperature
    if (root.containsKey("color_temp")) {
        state.color_temp = root["color_temp"];
        Serial.printf("  - DEBUG: Got color_temp: %i\n", state.color_temp);
    } 

    // White value
    if (root.containsKey("white_value")) {
        state.white_value = root["white_value"];
        Serial.printf("  - DEBUG: Got white_value: %i\n", state.white_value);
    }

    // Transition
    if (root.containsKey("transition")) {
        state.transition = (uint16_t) root["transition"];
        Serial.printf("  - DEBUG: Got transition: %i\n", state.transition);
    }

    return state;
}

const char* createJsonString(LightState& state) {
    StaticJsonBuffer<420> jsonBuffer;
    JsonObject& object = jsonBuffer.createObject();
    JsonObject& color = jsonBuffer.createObject();

    color["r"] = state.color.r;
    color["g"] = state.color.g;
    color["b"] = state.color.b;
    color["x"] = state.color.x;
    color["y"] = state.color.y;
    color["h"] = state.color.h;
    color["s"] = state.color.s;

    object["state"]       = (state.state) ? "ON" : "OFF"; 
    object["brightness"]  = state.brightness;
    object["color_temp"]  = state.color_temp;
    object["white_value"] = state.white_value;
    object["color"]       = color;
    object["effect"]      = state.effect.c_str();

    String output;
    object.printTo(output);
    return (const char*) output.c_str();
}