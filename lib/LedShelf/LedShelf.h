/**
 *  Creating a library file for LedShelf to clean up main file
 */
typedef struct Config {
  String ssid;
  String psk;
  String server;
  int  port;
  String username;
  String password;
  String client;
  String command_topic;
  String state_topic;
  String status_topic;
} Config;

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
  uint8_t color_temp; 
  uint8_t white_value;
  uint16_t transition;
  Color color;
  String effect;
  bool state;
} LightState;
