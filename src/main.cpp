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
// define MQTT_MAX_PACKET_SIZE 256
// define MQTT_MAX_TRANSFER_SIZE 256

#include <debug.h>
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <LedShelf.h>
#include <LightStateController.h>

// led GPIOs
static const uint8_t GPIO_RED   = 25;
static const uint8_t GPIO_GREEN = 26;
static const uint8_t GPIO_BLUE  = 27;

// define pwm output channels
static const uint8_t LEDC_RED   = 1;
static const uint8_t LEDC_GREEN = 2;
static const uint8_t LEDC_BLUE  = 3;

// global objects
String                ca_root;
Config                config;
LightStateController  lightState;
WiFiClientSecure      wifiClient;
PubSubClient          mqttClient;

uint8_t counter = 0;
uint8_t color   = 0;          // a value from 0 to 255 representing the hue
uint32_t R, G, B;           // the Red Green and Blue color components
uint8_t brightness = 255;

void readConfig()
{
    static const char* path = "/config.json";
    // String data;

    Serial.printf("Reading config file: %s\n", path);

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
    static const char* path = "/ca.pem";
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

void mqttPublishState()
{
    char json[256];
    lightState.printStateJsonTo(json);
    mqttClient.publish(config.state_topic.c_str(), json, true);
}

void mqttCallback (char* p_topic, byte* p_message, unsigned int p_length)
{
    digitalWrite(BUILTIN_LED, HIGH);

    Serial.printf("INFO: New MQTT message: '%s'\n", p_topic);

    if (!String(config.command_topic).equals(p_topic)) {
        Serial.printf("  - ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
        digitalWrite(BUILTIN_LED, LOW);
        return;
    }

    LightState newState = lightState.newState(p_message);
    // Handles the light setting logic. Should be moved to own function / object
    if (newState.state == true) {
        if (newState.status.hasEffect == false) {
            setLedToRGB(newState.color.r, newState.color.g, newState.color.b);
        }
    }
    else {
        setLedToRGB(0, 0, 0);
    }

    mqttPublishState();
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

    mqttPublishState();
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
    if (counter > 255) counter = 0;

    hueToRGB(counter, brightness);
    ledcWrite(1, R);
    ledcWrite(2, G);
    ledcWrite(3, B);
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting...\n");
    //  lightState.setCurrentState("hello");

    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, LOW);
    delay(100);

    if (!SPIFFS.begin()) {
        Serial.println("  - Could not mount SPIFFS.");
        return;
    }

    uint8_t result = lightState.initialize();
    LightState currentState  = lightState.getCurrentState();
    String resultString = "";

    if (result == LIGHT_STATEFILE_NOT_FOUND)      resultString = "file not found";
    if (result == LIGHT_STATEFILE_JSON_FAILED)    resultString = "json failed";
    if (result == LIGHT_STATEFILE_PARSED_SUCCESS) resultString = "success";

    Serial.printf("  - state initialize: %s\n", resultString.c_str());
    #ifdef DEBUG
    Serial.printf("  - DEBUG: current brightness: %i\n", currentState.brightness);
    Serial.printf("  - DEBUG: current color_temp: %i\n", currentState.color_temp);
    Serial.printf("  - DEBUG: current color: [%i, %i, %i, %0.2f, %0.2f]\n",
        currentState.color.r, currentState.color.g, currentState.color.b,
        currentState.color.h, currentState.color.s
    );
    Serial.printf("  - DEBUG: effect is set to '%s'\n", currentState.effect.c_str());
    #endif

    readConfig();
    readCA();
    setupWifi();
    setupMQTT();
    setupLeds();

    // set initial state of led after setup.
    if (currentState.state == true) {
        setLedToRGB(currentState.color.r, currentState.color.g, currentState.color.b);
    } else {
        setLedToRGB(0, 0, 0);
    }
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

    LightState currentState = lightState.getCurrentState();

    if (currentState.effect == "colorloop") {
        runEffectColorloop();
    }

    delay(33);
}
