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

// global objects
StaticJsonBuffer<512> configBuffer;
StaticJsonBuffer<420> currentLedState;
JsonVariant config;
JsonVariant ledState;

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

  config = configBuffer.parseObject(file);

  if (!config.success()) {
    Serial.println("  - parsing jsonBuffer failed");
    return;
  }

  file.close();
}

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

void setupWifi() 
{
  const char* ssid   = config["ssid"];
  const char* psk    = config["psk"];
  
  digitalWrite(BUILTIN_LED, LOW);

  delay(10);

  Serial.printf("Connecting to: %s ", ssid);
  
  WiFi.begin(ssid, psk);

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
  const char* server = config["server"];
  const int port = config["port"];

  wifiClient.setCACert(ca_root.c_str());

  Serial.printf("Setting up MQTT Client: %s %i\n", server, port);

  mqttClient.setClient(wifiClient);
  mqttClient.setServer(server, port); 
  mqttClient.setCallback(mqttCallback);
}

void connectMQTT() 
{
  const char* server      = config["server"];
  const char* username    = config["username"];
  const char* password    = config["password"];
  const char* client      = config["client"];
  const char* statusTopic = config["status_topic"];
  const char* commandTopic = config["command_topic"];
  const int   port        = config["port"];

  digitalWrite(BUILTIN_LED, HIGH);
  IPAddress mqttip;
  WiFi.hostByName(server, mqttip);

  Serial.printf("  - %s = ", server);
  Serial.println(mqttip);

  while (!mqttClient.connected()) {
    Serial.printf(
      "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\" \"%s\"... ", 
      server, port, username, password
    );
    
    if (mqttClient.connect(client, username, password, statusTopic, 0, true, "Disconnected")) {
      Serial.println(" connected");
      Serial.print("  - status: ");
      Serial.println(statusTopic);
      Serial.print("  - command: ");
      Serial.println(commandTopic);
      mqttClient.publish(statusTopic, "Online", true);
      mqttClient.subscribe(commandTopic);
    } 
    else {  
      Serial.print(" failed: ");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }

  digitalWrite(BUILTIN_LED, LOW);
}

/*
 * MQTT_json example:
 *  {
 *    "brightness": 255,
 *    "color_temp": 155,
 *    "color": {
 *      "r": 255,
 *      "g": 180,
 *      "b": 200,
 *      "x": 0.406,
 *      "y": 0.301,
 *      "h": 344.0,
 *      "s": 29.412
 *    },
 *    "effect": "colorloop",
 *    "state": "ON",
 *    "transition": 2,
 *    "white_value": 150
 *  }
 */
void mqttCallback (char* p_topic, byte* p_message, unsigned int p_length)
{
  String commandTopic = config["command_topic"];
  const char* stateTopic   = config["state_topic"];
  String message;

  digitalWrite(BUILTIN_LED, HIGH);

  for (uint8_t i = 0; i < p_length; i++) {
    message.concat( (char) p_message[i] );
  }

  Serial.print("INFO: New MQTT message. topic: ");
  Serial.println(p_topic);
  Serial.println("INFO: Message: ");
  Serial.println(message);

  if (commandTopic.equals(p_topic)) {
    StaticJsonBuffer<420> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(p_message);

    if (!root.success()) {
      Serial.println("ERROR: parseobject failed.");
      digitalWrite(BUILTIN_LED, LOW);
      return;
    }

    if (root.containsKey("state")) {
      String output;
      root.printTo(output);

      Serial.println("DEBUG: object got state");
      Serial.println("INFO: publishing new state:");
      Serial.println(output);

      bool state = (root["state"] == "ON") ? true : false;
      Serial.print("state is: ");
      Serial.println(state);

      mqttClient.publish(stateTopic, output.c_str(), true);
    }
  }

  digitalWrite(BUILTIN_LED, LOW);
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

  // Serial.printf("builtin led: %i\n", BUILTIN_LED);
  // Starting led setup
  // initialize the digital pin as an output.
  ledcAttachPin(red, 1);
  ledcAttachPin(green, 2);
  ledcAttachPin(blue, 3);

  ledcSetup(1, 12000, 8);
  ledcSetup(2, 12000, 8);
  ledcSetup(3, 12000, 8);
}

void loop() {
  const char* server = config["server"];

  // wait for WiFi connection
  if((WiFi.status() != WL_CONNECTED)) {
    Serial.println("WiFI not connected.");
    setupWifi();
    delay(500);
    return;
  }

  if (!mqttClient.connected()) {
    Serial.printf("Not connected to MQTT broker: %s\n", server);
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
