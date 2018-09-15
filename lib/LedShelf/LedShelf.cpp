#include "LedShelf.h"

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
    object["effect"]      = state.effect;

    String output = "";
    object.printTo(output);
    return output;
}