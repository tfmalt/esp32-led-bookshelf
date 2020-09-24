/**
 * Implementation of class for reading and accessing configuration
 */
#include "LedshelfConfig.h"
#include <SPIFFS.h>
#include <ArduinoJson.h>

void LedshelfConfig::setup()
{
    if (SPIFFS.begin())
    {
        Serial.println("  - Mounting SPIFFS file system.");
    }
    else
    {
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
    if (!file)
    {
        Serial.print("  - failed to open file (");
        Serial.print(configFile);
        Serial.println(") for reading");
        return;
    }

    // const size_t bufferSize = JSON_OBJECT_SIZE(20) + 300;

    Serial.printf("  - Reading config: %s\n", configFile.c_str());
    // Serial.printf("    - Buffersize: %i\n", bufferSize);

    DynamicJsonDocument config(2110);
    auto error = deserializeJson(config, file);

    file.close();

    if (error)
    {
        Serial.print("  - parsing jsonBuffer failed: ");
        Serial.println(error.c_str());
        return;
    }

    wifi_ssid = config["ssid"].as<char *>();
    wifi_psk = config["psk"].as<char *>();
    mqtt_server = config["server"].as<char *>();
    mqtt_port = config["port"].as<uint16_t>();
    mqtt_username = config["username"].as<char *>();
    mqtt_password = config["password"].as<char *>();
    mqtt_client = config["client"].as<char *>();
    mqtt_command_topic = config["command_topic"].as<char *>();
    mqtt_state_topic = config["state_topic"].as<char *>();
    mqtt_status_topic = config["status_topic"].as<char *>();
    mqtt_query_topic = config["query_topic"].as<char *>();
    mqtt_information_topic = config["information_topic"].as<char *>();
    mqtt_update_topic = config["update_topic"].as<char *>();
    mqtt_num_leds = config["num_leds"].as<uint16_t>();
    mqtt_milliamps = config["milliamps"].as<uint16_t>();

    Serial.print("    - ssid: ");
    Serial.println(wifi_ssid);
}

/**
 * Reads the TLS CA Root Certificate from file.
 */
void LedshelfConfig::readCAFile()
{
    Serial.printf("Reading CA file: %s\n", caFile.c_str());

    File file = SPIFFS.open(caFile, "r");

    if (!file)
    {
        Serial.printf("  - Failed to open %s for reading\n", caFile.c_str());
        return;
    }

    ca_root_data = "";
    while (file.available())
    {
        ca_root_data += file.readString();
    }

    file.close();
}

String LedshelfConfig::stateTopic()
{
    return String("/" + username + state_topic);
}

String LedshelfConfig::commandTopic()
{
    return String("/" + username + command_topic);
}

String LedshelfConfig::statusTopic()
{
    return String("/" + username + status_topic);
}

String LedshelfConfig::queryTopic()
{
    return String("/" + username + query_topic);
}

String LedshelfConfig::informationTopic()
{
    return String("/" + username + information_topic);
}

String LedshelfConfig::updateTopic()
{
    return String("/" + username + update_topic);
}