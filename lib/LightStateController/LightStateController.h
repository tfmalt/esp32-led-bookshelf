#include <Arduino.h>

struct Color {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  float x;
  float y;
  float h;
  float s;
};

struct LightState {
        uint8_t brightness;
        uint8_t color_temp;
        uint8_t white_value;
        uint16_t transition;
        struct Color color;
        const char* effect;
        bool state;
};

class LightStateController {
    private:
        struct LightState currentState;
    public:
        LightStateController();
        LightStateController(const char* stateString);
        bool setCurrentState(const char* stateString);
};