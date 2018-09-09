/*
 * A first attempt at writing code for ESP32.
 * This is going to be a led light controller for lights on my daughters book shelf.
 * 
 * MIT License
 * 
 * Copyright (c) 2018 Thomas Malt
 */

#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

// TLS Root Certificate to be read from SPIFFS
String ca_root;

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

Config config;

// global objects
WiFiClientSecure wifiClient;
PubSubClient mqttClient;

uint8_t counter = 0;

// led GPIOs
uint8_t red   = 25;
uint8_t green = 26;
uint8_t blue  = 27;

uint8_t color = 0;          // a value from 0 to 255 representing the hue
uint32_t R, G, B;           // the Red Green and Blue color components
uint8_t brightness = 255;

void readConfig()
{
  const char* path = "/config.json";
  String data;

  Serial.printf("Reading config file: %s\r\n", path);

  File file = SPIFFS.open(path, "r");
  if(!file) {
    Serial.println("  - failed to open file for reading");
    return;
  }

  StaticJsonBuffer<512> configBuffer;
  JsonObject& root = configBuffer.parseObject(file);

  if (!root.success()) {
    Serial.println("  - parsing jsonBuffer failed");
    return;
  }

  config.ssid          = (const char*) root["ssid"];
  config.psk           = (const char*) root["psk"];
  config.server        = (const char*) root["server"];
  config.port          = (int)         root["port"];
  config.username      = (const char*) root["username"];
  config.password      = (const char*) root["password"];
  config.client        = (const char*) root["client"];
  config.command_topic = (const char*) root["command_topic"];
  config.state_topic   = (const char*) root["state_topic"];
  config.status_topic  = (const char*) root["status_topic"];

  file.close();
}

/**
 * Reads the TLS CA Root Certificate from file.
 */
void readCA() 
{
  const char* path = "/ca.pem";

  Serial.printf("Reading CA file %s\n", path);

  File file = SPIFFS.open(path, "r");

  if (!file) {
    Serial.printf("  - Failed to open %s for reading\n", path);
    return;
  }

  ca_root = "";
  while (file.available()) {
    ca_root += file.readString();
  }

  file.close();
}


/**
 * Configure and setup wifi
 */
void setupWifi() 
{
  digitalWrite(BUILTIN_LED, LOW);

  delay(10);

  Serial.printf("Connecting to: %s ", config.ssid.c_str());
  
  WiFi.begin(config.ssid.c_str(), config.psk.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(BUILTIN_LED, HIGH);
    Serial.print(".");
    delay(500);
    digitalWrite(BUILTIN_LED, LOW);
  }

  Serial.println(" WiFi connected");
  Serial.print("  - IP address: ");
  Serial.println(WiFi.localIP());
  Serial.print("  - DNS: ");
  Serial.println(WiFi.dnsIP());
}

void setupMQTT()
{
  wifiClient.setCACert(ca_root.c_str());

  Serial.printf("Setting up MQTT Client: %s %i\n", config.server.c_str(), config.port);

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(config.server.c_str(), config.port); 
  mqttClient.setCallback(mqttCallback);
}

void connectMQTT() 
{
  digitalWrite(BUILTIN_LED, HIGH);
  IPAddress mqttip;
  WiFi.hostByName(config.server.c_str(), mqttip);

  Serial.printf("  - %s = ", config.server.c_str());
  Serial.println(mqttip);

  while (!mqttClient.connected()) {
    Serial.printf(
      "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\" \"%s\"... ", 
      config.server.c_str(), config.port, config.username.c_str(), config.password.c_str()
    );
    
    if (mqttClient.connect(
      config.client.c_str(), 
      config.username.c_str(), 
      config.password.c_str(), 
      config.status_topic.c_str(), 
      0, true, "Disconnected")
    ) {
      Serial.println(" connected");
      Serial.print("  - status: ");
      Serial.println(config.status_topic);
      Serial.print("  - command: ");
      Serial.println(config.command_topic);
      mqttClient.publish(config.status_topic.c_str(), "Online", true);
      mqttClient.subscribe(config.command_topic.c_str());
    } 
    else {  
      Serial.print(" failed: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }

  digitalWrite(BUILTIN_LED, LOW);
}


String mqttPaylodToString(byte* p_payload, unsigned int p_length)
{
  String message;

  for (uint8_t i = 0; i < p_length; i++) {
    message.concat( (char) p_payload[i] );
  }

  return message;
}

void mqttCallback (char* p_topic, byte* p_message, unsigned int p_length)
{
  String message = mqttPaylodToString(p_message, p_length);

  digitalWrite(BUILTIN_LED, HIGH);
  Serial.printf("INFO: New MQTT message: '%s'\n", p_topic);
  Serial.println("  - INFO: Message: ");
  Serial.println(message);

  if (!String(config.command_topic).equals(p_topic)) {
    Serial.printf("  - ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
    return;
  }

  if (String(config.command_topic).equals(p_topic)) {
    StaticJsonBuffer<420> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(p_message);

    if (!root.success()) {
      Serial.println("ERROR: parseobject failed.");
      digitalWrite(BUILTIN_LED, LOW);
      return;
    }

    if (root.containsKey("state")) {
      String output;
      LightState newState = {0};

      root.printTo(output);

      Serial.println("  - DEBUG: Got valid state in object");
      Serial.println(output);

      newState.state = (root["state"] == "ON") ? true : false;

      if (root.containsKey("color")) {
        JsonObject& color = root["color"].as<JsonObject>();
        if (!color.success()) {
          Serial.println("ERROR: invalid color statement found. Skipping");
          digitalWrite(BUILTIN_LED, LOW);
          return;
        }

        Serial.println("Object got color.");
        if (color.containsKey("r")) newState.color.r = color["r"];
        if (color.containsKey("g")) newState.color.g = color["g"];
        if (color.containsKey("b")) newState.color.b = color["b"]; 
        if (color.containsKey("x")) newState.color.b = color["x"]; 
        if (color.containsKey("y")) newState.color.b = color["y"]; 
        if (color.containsKey("h")) newState.color.b = color["h"]; 
        if (color.containsKey("s")) newState.color.b = color["s"]; 
      } 
      else {
        Serial.println("  - DEBUG: Did not get color");
      }

      Serial.print("state is: ");
      Serial.println(newState.state);
      Serial.print("R:");
      Serial.print(newState.color.r);
      Serial.print(", G:");
      Serial.print(newState.color.g);
      Serial.print(", B:");
      Serial.print(newState.color.b);

      mqttClient.publish(config.state_topic.c_str(), output.c_str(), true);
    }
    else {
      Serial.println("WARN: Got something else. Stopping parsing.");
    }
  }

  digitalWrite(BUILTIN_LED, LOW);
}

void setupLeds()
{
  // Starting led setup
  // initialize the digital pin as an output.
  ledcAttachPin(red, 1);
  ledcAttachPin(green, 2);
  ledcAttachPin(blue, 3);

  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);

}

void setup()
{

  Serial.begin(115200);
  Serial.printf("Starting...\n");

  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, LOW);
  delay(100);

  if (!SPIFFS.begin()) {
    Serial.println("  - Could not mount SPIFFS.");
    return;
  }


  readConfig();
  readCA();
  setupWifi();
  setupMQTT();
  setupLeds();
}

void loop() {
  // wait for WiFi connection
  if((WiFi.status() != WL_CONNECTED)) {
    Serial.println("WiFI not connected.");
    setupWifi();
    delay(500);
    return;
  }

  if (!mqttClient.connected()) {
    Serial.printf("Not connected to MQTT broker: %s\n", config.server.c_str());
    connectMQTT();
  }

/*
  Serial.print("Free heap: ");
  Serial.print(ESP.getFreeHeap()); // 182796
  Serial.print(", Cycle count: ");
  Serial.println(ESP.getCycleCount());
*/

  mqttClient.loop();

  counter++;

  setLed();
  delay(10);
}

void setLed() {
  if (counter > 255) {
    counter = 0;
  } 

  hueToRGB(counter, brightness);

  // Serial.printf("counter: %i, [%i, %i, %i]\n", counter, R, G, B);

  ledcWrite(1, R);
  ledcWrite(2, G);
  ledcWrite(3, B);
}

// Courtesy http://www.instructables.com/id/How-to-Use-an-RGB-LED/?ALLSTEPS
// function to convert a color to its Red, Green, and Blue components.
void hueToRGB(uint8_t hue, uint8_t brightness)
{
    uint16_t scaledHue = (hue * 6);
    uint8_t segment = scaledHue / 256; // segment 0 to 5 on the color wheel
    uint16_t segmentOffset = scaledHue - (segment * 256); // position within the segment

    uint8_t complement = 0;
    uint16_t prev = (brightness * ( 255 -  segmentOffset)) / 256;
    uint16_t next = (brightness *  segmentOffset) / 256;

    switch (segment) {
    case 0:      // red
        R = brightness;
        G = next;
        B = complement;
    break;
    case 1:     // yellow
        R = prev;
        G = brightness;
        B = complement;
    break;
    case 2:     // green
        R = complement;
        G = brightness;
        B = next;
    break;
    case 3:    // cyan
        R = complement;
        G = prev;
        B = brightness;
    break;
    case 4:    // blue
        R = next;
        G = complement;
        B = brightness;
    break;
   case 5:      // magenta
    default:
        R = brightness;
        G = complement;
        B = prev;
    break;
    }
}
