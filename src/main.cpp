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

uint8_t  gHue = 0; // rotating "base color" used by many of the patterns
uint16_t commandFrames     = FPS;
uint16_t commandFrameCount = 0;
ulong    commandStart      = 0;

// A typedef to hold a pointer to the current command to execute.
typedef void (*LightCommand)();

void cmdEmpty()
{
}

LightCommand currentCommand = cmdEmpty;
LightCommand currentEffect  = cmdEmpty;
//
// void cmdSetBrightness()
// {
//     LightState state = lightState.getCurrentState();
//
//     uint8_t  target   = state.brightness;
//     uint8_t  current  = FastLED.getBrightness();
//
//     if (commandFrameCount < commandFrames) {
//         FastLED.setBrightness(current + ((target - current) / (commandFrames - commandFrameCount)));
//         commandFrameCount++;
//     } else {
//         commandFrameCount = 0;
//         commandFrames     = FPS;
//         currentCommand    = cmdEmpty;
//
//         Serial.printf(
//             "  - command: setting brightness DONE [%i] %lu ms.\n",
//             FastLED.getBrightness(), (millis() - commandStart)
//         );
//     }
// }

void cmdHandleEffects()
{
    effects.runCurrentCommand();
}
/**
 * Helper function that blends one uint8_t toward another by a given amount
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
    if( cur == target) return;

    if( cur < target ) {
        uint8_t delta = target - cur;
        delta = scale8_video( delta, amount);
        cur += delta;
    } else {
        uint8_t delta = cur - target;
        delta = scale8_video( delta, amount);
        cur -= delta;
    }
}

/**
 * Blend one CRGB color toward another CRGB color by a given amount.
 * Blending is linear, and done in the RGB color space.
 * This function modifies 'cur' in place.
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
CRGB fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount)
{
    nblendU8TowardU8( cur.red,   target.red,   amount);
    nblendU8TowardU8( cur.green, target.green, amount);
    nblendU8TowardU8( cur.blue,  target.blue,  amount);
    return cur;
}

/**
 * Fade an entire array of CRGBs toward a given background color by a given amount
 * This function modifies the pixel array in place.
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void fadeTowardColor( CRGB* L, uint16_t N, const CRGB& bgColor, uint8_t fadeAmount)
{
    uint16_t check = 0;
    for( uint16_t i = 0; i < N; i++) {
        fadeTowardColor( L[i], bgColor, fadeAmount);
        if (L[i] == bgColor) check++;
    }

    if (check == NUM_LEDS) {
        currentCommand = cmdEmpty;
        Serial.printf("  - fade towards color done in %lu ms.\n", (millis() - commandStart));
    }
}


void cmdFadeTowardColor()
{
    LightState state = lightState.getCurrentState();
    CRGB targetColor(state.color.r, state.color.g, state.color.b);

    fadeTowardColor(leds, NUM_LEDS, targetColor, 12);
}

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

    // Handles the light setting logic. Should be moved to own function / object
    if (newState.state == true) {
        if(newState.status.hasBrightness)
        {
            Serial.printf("  - Got new brightness: '%i'\n", newState.brightness);
            effects.setCurrentCommand(Effects::Command::Brightness);
            currentCommand = cmdHandleEffects;
            // commandFrames = (FPS * (newState.transition || 1));
            // commandStart = millis();
        }
        else if(newState.status.hasColor) {
            Serial.println("  - Got color");
            // fill_solid(leds, NUM_LEDS, CRGB(newState.color.r, newState.color.g, newState.color.b));
            currentCommand = cmdFadeTowardColor;
            commandStart = millis();
        }
        else if (newState.status.hasEffect && newState.effect == "") {
            Serial.println("  - Got effect none. fading to current color.");
            currentCommand = cmdFadeTowardColor;
            commandStart = millis();
        }
        // else if (newState.status.hasEffect && newState.effect != "") {
        //     Serial.printf("  - Got effect '%s'. Setting it.\n", newState.effect);
        //     currentEffect =
        // }
        else {
            // assuming turn on is the only thing.
            effects.setCurrentCommand(Effects::Command::Brightness);
            currentCommand = cmdHandleEffects;
            // currentCommand = cmdSetBrightness;
            // commandFrames = (FPS * (newState.transition || 1));
            // commandStart = millis();
        }
    }
    else {
        Serial.println("  - Told to turn off");
        FastLED.setBrightness(0);
    }

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

    uint8_t result           = lightState.initialize();
    LightState currentState  = lightState.getCurrentState();
    effects.setFPS(FPS);
    effects.setLightStateController(&lightState);

    String resultString      = "";


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

    // all examples I've seen has a startup grace delay.
    // Just cargo-cult copying that practise.
    delay(3000);

    FastLED.addLeds<WS2812B, GPIO_DATA, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

    FastLED.setBrightness(currentState.brightness);
    currentCommand = cmdFadeTowardColor;

    int core = xPortGetCoreID();
    Serial.print("Main code running on core ");
    Serial.println(core);

    // -- Create the FastLED show task
    xTaskCreatePinnedToCore(
        FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2,
        &FastLEDshowTaskHandle, FASTLED_SHOW_CORE
    );
}

void rainbow()
{
    // FastLED's built-in rainbow generator
    fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void addGlitter( fract8 chanceOfGlitter)
{
    if( random8() < chanceOfGlitter) {
        leds[ random16(NUM_LEDS) ] += CRGB::White;
    }
}

void rainbowWithGlitter()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    rainbow();
    addGlitter(80);
}

void confetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 20);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void sinelon()
{
  fadeToBlackBy( leds, NUM_LEDS, 96 );
  LightState state = lightState.getCurrentState();

  int pos = beatsin16(18, 0, NUM_LEDS-1 );
  // leds[pos] += CHSV( gHue, 255, 255);
  leds[pos] += CRGB(state.color.r, state.color.g, state.color.b);
}

void bpm()
{
    // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
    uint8_t BeatsPerMinute = 64;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < NUM_LEDS; i++) { //9948
        leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
    }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
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

    currentCommand(); // a call to a function pointer.
    currentEffect();  // a call to a function pointer.
    if (currentState.effect == "Glitter Rainbow") rainbowWithGlitter();
    if (currentState.effect == "Rainbow")         rainbow();
    if (currentState.effect == "BPM")             bpm();
    if (currentState.effect == "Confetti")        confetti();
    if (currentState.effect == "Juggle")          juggle();
    if (currentState.effect == "Sinelon")         sinelon();

    fastLEDshowESP32();
    // FastLED.show();
    delay(1000/FPS);
}
