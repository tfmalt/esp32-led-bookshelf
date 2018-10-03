/**
 * Implementation of class for reading and accessing configuration
 */
#include "LedshelfConfig.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>


void LedshelfConfig::setup()
{
    if (SPIFFS.begin()) {
        Serial.println("  - Mounting SPIFFS file system.");
    }
    else {
        Serial.println("  - ERROR: Could not mount SPIFFS.");
        ESP.restart();
        return;
    }

    parseConfigFile();
    readCAFile();
}

void LedshelfConfig::parseConfigFile()
{
    File file = SPIFFS.open(configFile, "r");
    if(!file) {
        Serial.println("  - failed to open file for reading");
        return;
    }
    const size_t bufferSize = JSON_OBJECT_SIZE(10) + 300;

    StaticJsonBuffer<bufferSize> configBuffer;
    JsonObject& root = configBuffer.parseObject(file);

    if (!root.success()) {
        Serial.println("  - parsing jsonBuffer failed");
        return;
    }

    wifi_ssid          = root["ssid"].as<char*>();
    wifi_psk           = root["psk"].as<char*>();
    mqtt_server        = root["server"].as<char*>();
    mqtt_port          = root["port"].as<uint16_t>();
    mqtt_username      = root["username"].as<char*>();
    mqtt_password      = root["password"].as<char*>();
    mqtt_client        = root["client"].as<char*>();
    mqtt_command_topic = root["command_topic"].as<char*>();
    mqtt_state_topic   = root["state_topic"].as<char*>();
    mqtt_status_topic  = root["status_topic"].as<char*>();

    file.close();
}

/**
 * Reads the TLS CA Root Certificate from file.
 */
void LedshelfConfig::readCAFile()
{
    Serial.printf("Reading CA file: %s\n", caFile.c_str());

    File file = SPIFFS.open(caFile, "r");

    if (!file) {
        Serial.printf("  - Failed to open %s for reading\n", caFile.c_str());
        return;
    }

    ca_root_data = "";
    while (file.available()) {
        ca_root_data += file.readString();
    }

    file.close();
}
