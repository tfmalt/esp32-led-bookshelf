
#ifndef LightStateController_hpp
#define LightStateController_hpp

#include <Arduino.h>
#include <ArduinoJson.h>

#include <map>
#include <string>

#define LIGHT_STATEFILE_PARSED_SUCCESS 0
#define LIGHT_STATEFILE_NOT_FOUND 1
#define LIGHT_STATEFILE_JSON_FAILED 2
#define LIGHT_MQTT_JSON_FAILED 3
#define LIGHT_MQTT_JSON_NO_STATE 4
#define LIGHT_STATEFILE_WROTE_SUCCESS 5

namespace LightState {

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
  std::string effect;
  bool state;
  LightStatus status;
} LightState;

class Controller {
 public:
  Controller() {
    Color defaultColor = {0};

    LightStatus defaultStatus = {false};
    defaultStatus.status = 255;

    defaultState.color = defaultColor;
    defaultState.effect = "colorloop";
    defaultState.brightness = 255;
    defaultState.transition = 1;
    defaultState.state = false;
    defaultState.status = defaultStatus;
  };

  uint8_t initialize();
  // LightState::Controller &setCurrentState(const char *stateString);
  LightState &parseNewState(std::string data);
  LightState &getCurrentState();
  std::string getCurrentStateAsJSON();
  void serializeCurrentState(char *output, int length);
  void handleNewState(byte *payload);

 private:
  std::map<String, LightState> lightStates;
  LightState currentState = {0};
  LightState defaultState = {0};
  ulong timestamp = 0;

  const char *stateFile = "/light_state.json";

  LightState getLightStateFromPayload(std::string payload);
  void printStateDebug(LightState &state);
  uint8_t saveCurrentState();
};
}  // namespace LightState
#endif  // LightStateController_h