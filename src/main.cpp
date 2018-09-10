/*
 * A first attempt at writing code for ESP32.
 * This is going to be a led light controller for lights on my daughters book shelf.
 * 
 * - Uses MQTT over TLS to talk to a MQTT broker.
 * - Implementes the mqtt_json interface to get commands from a home assistant 
 *   server. (https://home-assistant.io)
 * - Uses SPIFFS to store configuration and separate configuration from firmware
 * 
 * MIT License
 * 
 * Copyright (c) 2018 Thomas Malt
 */

#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LedShelf.h>

// TLS Root Certificate to be read from SPIFFS
String ca_root;

// global objects
Config           config;
LightState       currentState;
WiFiClientSecure wifiClient;
PubSubClient     mqttClient;

uint8_t counter = 0;

// led GPIOs
#define GPIO_RED   25
#define GPIO_GREEN 26
#define GPIO_BLUE  27

#define LEDC_RED    1
#define LEDC_GREEN  2
#define LEDC_BLUE   3

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

  Serial.printf("Reading CA file: %s\n", path);

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

void setLedToRGB(uint8_t r, uint8_t g, uint8_t b) {
  Serial.printf("DEBUG: Setting led to color: [%i, %i, %i]\n", r, g, b);
  ledcWrite(LEDC_RED, r);
  ledcWrite(LEDC_GREEN, g);
  ledcWrite(LEDC_BLUE, b);
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
  Serial.print("INFO: Message: ");
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

      newState.state = (root["state"] == "ON") ? true : false;
      Serial.printf("  - DEBUG: Got valid state: %i\n", newState.state);

      if (root.containsKey("effect")) {
        newState.effect = (const char*) root["effect"];
        Serial.printf("  - DEBUG: Got effect: '%s'\n", newState.effect.c_str());
        currentState.effect = newState.effect;
      } else {
        newState.effect = ""; // make sure we nuke effect when there is none.
        currentState.effect = newState.effect;
      }

      if (root.containsKey("color")) {
        JsonObject& color = root["color"].as<JsonObject>();
        if (!color.success()) {
          Serial.println("ERROR: invalid color statement found. Skipping");
          digitalWrite(BUILTIN_LED, LOW);
          return;
        }

        if (color.containsKey("r")) newState.color.r = (uint8_t) color["r"];
        if (color.containsKey("g")) newState.color.g = (uint8_t) color["g"];
        if (color.containsKey("b")) newState.color.b = (uint8_t) color["b"]; 
        if (color.containsKey("x")) newState.color.x = (float) color["x"]; 
        if (color.containsKey("y")) newState.color.y = (float) color["y"]; 
        if (color.containsKey("h")) newState.color.h = (float) color["h"]; 
        if (color.containsKey("s")) newState.color.s = (float) color["s"]; 

        Serial.printf(
          "  - DEBUG: Got color: R:%i, G:%i, B:%i,   X:%0.2f, Y:%0.2f,   H:%0.2f, S:%0.2f\n", 
          newState.color.r, newState.color.g, newState.color.b, newState.color.x, 
          newState.color.y, newState.color.h, newState.color.s
        );

        setLedToRGB(newState.color.r, newState.color.g, newState.color.b);
      } 

      if (root.containsKey("brightness")) {
        newState.brightness = root["brightness"];
        Serial.printf("  - DEBUG: Got brightness: %i\n", newState.brightness);
      } 

      if (root.containsKey("color_temp")) {
        newState.color_temp = root["color_temp"];
        Serial.printf("  - DEBUG: Got color_temp: %i\n", newState.color_temp);
      } 

      if (root.containsKey("white_value")) {
        newState.white_value = root["white_value"];
        Serial.printf("  - DEBUG: Got white_value: %i\n", newState.white_value);
      }

      if (root.containsKey("transition")) {
        newState.transition = (uint16_t) root["transition"];
        Serial.printf("  - DEBUG: Got transition: %i\n", newState.transition);
      }

      mqttClient.publish(config.state_topic.c_str(), output.c_str(), true);
    }
    else {
      Serial.println("WARN: Got something else. Stopping parsing.");
    }
  }

  digitalWrite(BUILTIN_LED, LOW);
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
      "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\" ... ", 
      config.server.c_str(), config.port, config.username.c_str() 
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


void setupLeds()
{
  // Starting led setup
  // initialize the digital pin as an output.
  ledcAttachPin(GPIO_RED,   LEDC_RED);
  ledcAttachPin(GPIO_GREEN, LEDC_GREEN);
  ledcAttachPin(GPIO_BLUE,  LEDC_BLUE);

  ledcSetup(LEDC_RED,   12000, 8);
  ledcSetup(LEDC_GREEN, 12000, 8);
  ledcSetup(LEDC_BLUE,  12000, 8);
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

void runEffectColorloop() {
  counter++;
  if (counter > 255) {
    counter = 0;
  } 

  hueToRGB(counter, brightness);

  // Serial.printf("counter: %i, [%i, %i, %i]\n", counter, R, G, B);

  ledcWrite(1, R);
  ledcWrite(2, G);
  ledcWrite(3, B);
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

  currentState = {0}; // initialize state with all zeros.
  currentState.effect = "colorloop";

  Serial.printf("  - INFO: effect is set to '%s'\n", currentState.effect.c_str());

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

  mqttClient.loop();

  if (currentState.effect.equals("colorloop")) {
    runEffectColorloop();
  }
  delay(33);
}
