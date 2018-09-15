
#ifndef LightStateController_h
#define LightStateController_h

#include <Arduino.h>

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

typedef struct LightState {
    uint8_t brightness;
    uint8_t white_value;
    uint16_t color_temp;
    uint16_t transition;
    struct Color color;
    const char* effect;
    bool state;
} LightState;

typedef struct LightStatus {
    bool hasBrightness;
    bool hasWhiteValue;
    bool hasColorTemp;
    bool hasTransition;
    bool hasColor;
    bool hasEffect;
    bool hasState;
} LightStatus;

class LightStateController {
    private:
        LightState currentState = {0};
        const char* stateFile = "/lightState.json";
        uint8_t parseStateFile();
    public:
        LightStateController();
        LightStateController(const char* stateString);
        bool setCurrentState(const char* stateString);
        uint8_t newState(byte* payload);
};

#endif // LightStateControlller_h