/**
 *  Creating a library file for LedShelf to clean up main file
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LightStateController.h>

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


Color getColorFromJson(JsonObject& root);
LightState getLightStateFromMQTT(byte * message);
String createJsonString(LightState& state);