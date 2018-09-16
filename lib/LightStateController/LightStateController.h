
#ifndef LightStateController_h
#define LightStateController_h

#include <Arduino.h>
#include <ArduinoJson.h>

#define LIGHT_STATEFILE_PARSED_SUCCESS  0
#define LIGHT_STATEFILE_NOT_FOUND       1
#define LIGHT_STATEFILE_JSON_FAILED     2
#define LIGHT_MQTT_JSON_FAILED          3
#define LIGHT_MQTT_JSON_NO_STATE        4

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

typedef struct LightState {
    uint8_t brightness;
    uint8_t white_value;
    uint16_t color_temp;
    uint16_t transition;
    Color color;
    const char* effect;
    bool state;
    LightStatus status;
} LightState;

class LightStateController {
    private:
        LightState  currentState = {0};
        LightState  defaultState = {0};

        const char* stateFile = "/lightState.json";

        Color       getColorFromJsonObject(JsonObject& root);
        LightState  getLightStateFromPayload(byte* payload);
        void        printStateDebug(LightState& state);
    public:
        LightStateController();
        uint8_t     initialize();
        bool        setCurrentState(const char* stateString);
        LightState  newState(byte* payload);
        LightState  getCurrentState();
        const char* getStateJson();
};

#endif // LightStateControlller_h