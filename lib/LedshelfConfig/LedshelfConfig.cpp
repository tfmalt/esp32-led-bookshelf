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

    Serial.printf("  - Reading config: %s\n", configFile);

    StaticJsonDocument<680> config;
    auto error = deserializeJson(config, file);

    file.close();

    if (error)
    {
        Serial.print("  - parsing jsonBuffer failed: ");
        Serial.println(error.c_str());
        return;
    }

    strlcpy(wifi_ssid, config["wifi"]["ssid"] | "", sizeof(wifi_ssid));
    strlcpy(wifi_psk, config["wifi"]["psk"] | "", sizeof(wifi_psk));
    strlcpy(mqtt_server, config["mqtt"]["server"] | "", sizeof(mqtt_server));
    mqtt_port = config["mqtt"]["port"] | 1883;
    mqtt_ssl = config["mqtt"]["ssl"] | false;
    strlcpy(mqtt_username, config["mqtt"]["username"] | "", sizeof(mqtt_username));
    strlcpy(mqtt_password, config["mqtt"]["password"] | "", sizeof(mqtt_password));
    strlcpy(mqtt_client, config["mqtt"]["client"] | "", sizeof(mqtt_client));
    strlcpy(mqtt_command_topic, config["mqtt"]["command_topic"] | "", sizeof(mqtt_command_topic));
    strlcpy(mqtt_state_topic, config["mqtt"]["state_topic"] | "", sizeof(mqtt_state_topic));
    strlcpy(mqtt_status_topic, config["mqtt"]["status_topic"] | "", sizeof(mqtt_status_topic));
    strlcpy(mqtt_query_topic, config["mqtt"]["query_topic"] | "", sizeof(mqtt_query_topic));
    strlcpy(mqtt_information_topic, config["mqtt"]["information_topic"] | "", sizeof(mqtt_information_topic));
    strlcpy(mqtt_update_topic, config["mqtt"]["update_topic"] | "", sizeof(mqtt_update_topic));
    fastled_num_leds = config["fastled"]["num_leds"] | 0;
    fastled_milliamps = config["fastled"]["milliamps"] | 0;

    Serial.print("    - ssid: ");
    Serial.println(wifi_ssid);
}

/**
 * Reads the TLS CA Root Certificate from file.
 */
void LedshelfConfig::readCAFile()
{
    Serial.printf("Reading CA file: %s\n", caFile);

    File file = SPIFFS.open(caFile, "r");

    if (!file)
    {
        Serial.printf("  - Failed to open %s for reading\n", caFile);
        return;
    }

    strcpy(ca_root, "");
    while (file.available())
    {
        strcat(ca_root, file.readString().c_str());
    }

    file.close();
}

void LedshelfConfig::stateTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_state_topic);
}

void LedshelfConfig::commandTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_command_topic);
}

void LedshelfConfig::statusTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_status_topic);
}

void LedshelfConfig::queryTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_query_topic);
}

void LedshelfConfig::informationTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_information_topic);
}

void LedshelfConfig::updateTopic(char *topic)
{
    strcpy(topic, "/");
    strcat(topic, mqtt_username);
    strcat(topic, mqtt_update_topic);
}