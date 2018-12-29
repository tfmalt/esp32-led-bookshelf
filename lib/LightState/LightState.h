
#ifndef LightState_h
#define LightState_h

#include <Arduino.h>
#include <ArduinoJson.h>

#define LIGHT_STATEFILE_PARSED_SUCCESS  0
#define LIGHT_STATEFILE_NOT_FOUND       1
#define LIGHT_STATEFILE_JSON_FAILED     2
#define LIGHT_MQTT_JSON_FAILED          3
#define LIGHT_MQTT_JSON_NO_STATE        4
#define LIGHT_STATEFILE_WROTE_SUCCESS   5

typedef struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    float x;
    float y;
    float h;
    float s;
} Color;

typedef struct LightStatus {
    bool hasBrightness;
    bool hasWhiteValue;
    bool hasColorTemp;
    bool hasTransition;
    bool hasColor;
    bool hasEffect;
    bool hasState;
    bool success;
    uint8_t status;
} LightStatus;

typedef struct LightStateData {
    uint8_t brightness;
    uint8_t white_value;
    uint16_t color_temp;
    uint16_t transition;
    Color color;
    String effect;
    bool state;
    LightStatus status;
} LightStateData;

class LightState {
    public:
        LightState();
        uint8_t         initialize(const char* stateFile);
        LightStateData& parseNewState(byte* payload);
        LightStateData& getCurrentState();
        void            printStateJsonTo(char* output);

    private:
        LightStateData  currentState = {0};
        LightStateData  defaultState = {0};
        ulong           timestamp    = 0;

        char* stateFile;  // = "/light_state.json";

        Color           getColorFromJsonObject(JsonObject& root);
        LightStateData  getLightStateFromPayload(byte* payload);
        void            printStateDebug(LightState& state);
        JsonObject&     createCurrentStateJsonObject(JsonObject& object, JsonObject& color);
        uint8_t         saveCurrentState();
};

#endif // LightState_h