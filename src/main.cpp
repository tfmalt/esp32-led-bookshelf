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
#define FASTLED_ALLOW_INTERRUPTS 0

#include <debug.h>
#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <LedShelf.h>
#include <LightStateController.h>
#include <Effects.h>

FASTLED_USING_NAMESPACE

// Fastled definitions
static const uint8_t GPIO_DATA         = 18;
static const uint8_t NUM_LEDS          = 60;
static const uint8_t FPS               = 60;
static const uint8_t FASTLED_SHOW_CORE = 0;

// Using esp32 other core to run FastLED.show().
// -- Task handles for use in the notifications
static TaskHandle_t FastLEDshowTaskHandle   = 0;
static TaskHandle_t userTaskHandle          = 0;

// A command to hold the command we're currently executing.

// global objects
String                ca_root;        // CA Root certificate for TLS
Config                config;         // Struct parsed from json config file.
LightStateController  lightState;     // Own object. Responsible for state.
WiFiClientSecure      wifiClient;
PubSubClient          mqttClient;
CRGB                  leds[NUM_LEDS];
Effects               effects;

uint16_t commandFrames     = FPS;
uint16_t commandFrameCount = 0;
ulong    commandStart      = 0;

/**
 * show() for ESP32
 *
 * Call this function instead of FastLED.show(). It signals core 0 to issue a show,
 * then waits for a notification that it is done.
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
void fastLEDshowESP32()
{
    if (userTaskHandle == 0) {
        // -- Store the handle of the current task, so that the show task can
        //    notify it when it's done
        userTaskHandle = xTaskGetCurrentTaskHandle();

        // -- Trigger the show task
        xTaskNotifyGive(FastLEDshowTaskHandle);

        // -- Wait to be notified that it's done
        const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 200 );
        ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
        userTaskHandle = 0;
    }
}

/**
 * show Task
 * This function runs on core 0 and just waits for requests to call FastLED.show()
 *
 * Borrowed from https://github.com/FastLED/FastLED/blob/master/examples/DemoReelESP32/DemoReelESP32.ino
 */
void FastLEDshowTask(void *pvParameters)
{
    // -- Run forever...
    for(;;) {
        // -- Wait for the trigger
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // -- Do the show (synchronously)
        FastLED.show();

        // -- Notify the calling task
        xTaskNotifyGive(userTaskHandle);
    }
}


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

uint8_t scaleHue(float h)
{
    return static_cast<uint8_t>(h*(256.0/360.0));
}

/**
 * Looks over the new state and dispatches updates to effects and issues
 * commands.
 */
void handleNewState(LightState state) {
    Serial.println("DEBUG: Got new state:");
    if (state.state == true) {
        if(state.status.hasBrightness)
        {
            Serial.printf("  - Got new brightness: '%i'\n", state.brightness);
            effects.setCurrentCommand(Effects::Command::Brightness);
        }
        else if(state.status.hasColor) {
            Serial.println("  - Got color");
            if (effects.getCurrentEffect() == Effects::Effect::NullEffect) {
                effects.setCurrentCommand(Effects::Command::Color);
            }
            else {
                Serial.printf(
                    "    - Effect is: '%s' hue: %.2f\n",
                    state.effect.c_str(), state.color.h
                );
                effects.setStartHue(scaleHue(state.color.h));
            }
        }
        else if (state.status.hasEffect) {
            Serial.printf("  - Got effect '%s'. Setting it.\n", state.effect.c_str());
            effects.setCurrentEffect(state.effect);
            if (state.effect == "") {
                effects.setCurrentCommand(Effects::Command::Color);
            }
        }
        else {
            // assuming turn on is the only thing.
            effects.setCurrentCommand(Effects::Command::Brightness);
        }
    }
    else {
        Serial.println("  - Told to turn off");
        FastLED.setBrightness(0);
    }
}

/**
 * Callback when we recive MQTT messages on topics we listen to.
 *
 * Parses message, dispatches commands and updates settings.
 */
void mqttCallback (char* p_topic, byte* p_message, unsigned int p_length)
{
    digitalWrite(BUILTIN_LED, HIGH);

    Serial.printf("INFO: New MQTT message: '%s'\n", p_topic);

    if (!String(config.command_topic).equals(p_topic)) {
        Serial.printf("  - ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
        digitalWrite(BUILTIN_LED, LOW);
        return;
    }

    LightState newState = lightState.parseNewState(p_message);
    handleNewState(newState);

    mqttPublishState();
    digitalWrite(BUILTIN_LED, LOW);
}
// End of MQTT Callback
// =========================================================================

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

void setupFastLED()
{
    Serial.println("Setting up LED");
    lightState.initialize();
    LightState  currentState    = lightState.getCurrentState();

    Serial.printf("  - state is: '%s'\n", currentState.state ? "On" : "Off");

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(currentState.state ? currentState.brightness : 0);

    effects.setFPS(FPS);
    effects.setLightStateController(&lightState);
    effects.setLeds(leds, NUM_LEDS);
    effects.setCurrentEffect(currentState.effect);
    effects.setStartHue(scaleHue(currentState.color.h));

    if (currentState.effect == "") {
        effects.setCurrentCommand(Effects::Command::Color);
    }

    // -- Create the FastLED show task
    xTaskCreatePinnedToCore(
        FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
        &FastLEDshowTaskHandle, FASTLED_SHOW_CORE
    );
}

void setup()
{
    Serial.begin(115200);
    Serial.printf("Starting...\n");

    if (SPIFFS.begin()) {
        Serial.println("  - Mounting SPIFFS file system.");
    }
    else {
        Serial.println("  - ERROR: Could not mount SPIFFS.");
        ESP.restart();
        return;
    }

    readConfig();
    readCA();
    setupWifi();
    setupMQTT();

    // all examples I've seen has a startup grace delay.
    // Just cargo-cult copying that practise.
    delay(3000);
    setupFastLED();
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

    effects.runCurrentCommand();
    effects.runCurrentEffect();

    fastLEDshowESP32();
    // FastLED.show();
    delay(1000/FPS);
}
