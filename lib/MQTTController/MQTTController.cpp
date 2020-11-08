
#ifdef IS_ESP32
#include "MQTTController.hpp"

#include <FastLED.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <functional>
#include <string>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

MQTTController &MQTTController::setup() {
#ifdef DEBUG
  Serial.printf("Setting up MQTT Client: %s %i\n", config.mqtt_server.c_str(),
                config.mqtt_port);
#endif

  wifiCtrl.setup();
  wifiCtrl.connect();

  config.statusTopic(statusTopic);
  config.commandTopic(commandTopic);
  config.updateTopic(updateTopic);
  config.stateTopic(stateTopic);
  config.queryTopic(queryTopic);
  config.informationTopic(informationTopic);

  timeClient.begin();
#ifdef DEBUG
  Serial.println("  - NTP time client started.");
#endif

  WiFiClient &wifi = wifiCtrl.getWiFiClient();
#ifdef DEBUG
  Serial.println("  - Got wifi client");
#endif

  bool setbuf = client.setBufferSize(512);

  Serial.printf("  - Set MQTT buffer to 512: %s\n",
                (setbuf) ? "true" : "false");

  client.setClient(wifi);
  client.setServer(config.mqtt_server.c_str(), config.mqtt_port);
  client.setCallback([this](char *p_topic, byte *p_message,
                            unsigned int p_length) {
    std::string topic = p_topic;
    std::string message(reinterpret_cast<const char *>(p_message), p_length);
    // callback_f(p_topic, p_message, p_length);
    _onMessage(topic, message);
  });

  return *this;
}

// ==========================================================================
// Event handler callbacks
// ==========================================================================
MQTTController &MQTTController::onMessage(
    std::function<void(std::string, std::string)> callback) {
  this->_onMessage = callback;
}

MQTTController &MQTTController::onReady(std::function<void()> callback) {
  this->_onReady = callback;
}

MQTTController &MQTTController::onDisconnect(std::function<void()> callback) {
  this->_onDisconnect = callback;
}

MQTTController &MQTTController::onError(
    std::function<void(std::string error)> callback) {
  this->_onError = callback;
}

void MQTTController::checkConnection() {
  if (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.println("Restarting because WiFI not connected.");
#endif

    this->_onError("Restarting because WiFi not connected.");
    delay(1000);
    ESP.restart();
    return;
  }
  if (!client.connected()) {
#ifdef DEBUG
    Serial.printf("MQTT broker not connected: %s\n",
                  config.mqtt_server.c_str());
#endif
    this->_onDisconnect();
    connect();
  }

  client.loop();
}

/**
 * Publish the current state
 */
void MQTTController::publishState() {
  if (this->lightState == nullptr) {
    Serial.println("ERROR: lightState not set.");
    return;
  }

  char json[256];
  lightState->serializeCurrentState(json, 256);

  client.publish(stateTopic, json, true);
}

void MQTTController::publishInformation(const char *message) {
  Serial.printf("GETTING READY: %s %s\n", informationTopic, message);

  client.publish(informationTopic, message, false);
  Serial.println("DONE");
}

/**
 * publish information data
 * compile some useful metainformation about the system
 */
void MQTTController::publishInformationData() {
  char *msg;

  Serial.println("PUBLISHINFORMATION DATA");

  asprintf(&msg,
           "{\"time\": \"%s\", \"hostname\": \"%s\", \"version\": \"%s\", "
           "\"uptime\": %lu, \"memory\": %d }",

           timeClient.getFormattedTime().c_str(), WiFi.getHostname(), version,
           millis(), xPortGetFreeHeapSize());

  const char *message = msg;

  Serial.println(message);
  Serial.println(strlen(message));

  publishInformation(message);
}

/**
 * Callback when we recive MQTT messages on topics we listen to.
 *
 * Parses message, dispatches commands and updates settings.
 */
void MQTTController::callback_f(char *p_topic, byte *p_message,
                                unsigned int p_length) {
#ifdef DEBUG
  Serial.printf("- MQTT Got topic: '%s'\n", p_topic);
#endif
  if (effects->currentCommandType == Effects::Command::FirmwareUpdate) {
    publishInformation("Firmware update active. Ignoring command.");
    return;
  }

  if (strcmp(commandTopic, p_topic) == 0) {
    // LightState& newState = lightState.parseNewState(p_message);
    handleNewState(lightState->parseNewState(p_message));
    publishState();
    return;
  }

  if (strcmp(updateTopic, p_topic) == 0) {
    handleUpdate();
    return;
  }
#ifdef DEBUG
  Serial.printf("- ERROR: Not a valid topic: '%s'. IGNORING\n", p_topic);
#endif
  return;
}
// End of MQTT Callback

/**
 * Looks over the new state and dispatches updates to effects and issues
 * commands.
 */
void MQTTController::handleNewState(LightState &state) {
  if (state.state == false) {
#ifdef DEBUG
    Serial.println("- Told to turn off");
#endif
    FastLED.setBrightness(0);
    return;
  }

  if (state.status.hasBrightness) {
#ifdef DEBUG
    Serial.printf("- Got new brightness: '%i'\n", state.brightness);
#endif
    effects->setCurrentCommand(Effects::Command::Brightness);
  } else if (state.status.hasColor) {
#ifdef DEBUG
    Serial.println("  - Got color");
#endif
    if (effects->getCurrentEffect() == Effects::Effect::NullEffect) {
      effects->setCurrentCommand(Effects::Command::Color);
    } else {
#ifdef DEBUG
      Serial.printf("    - Effect is: '%s' hue: %.2f\n", state.effect.c_str(),
                    state.color.h);
#endif
      effects->setStartHue(state.color.h);
    }
  } else if (state.status.hasEffect) {
#ifdef DEBUG
    Serial.printf("  - MQTT: Got effect '%s'. Setting it.\n",
                  state.effect.c_str());
#endif
    effects->setCurrentEffect(state.effect);
    if (state.effect == "") {
      effects->setCurrentCommand(Effects::Command::Color);
    }
  } else if (state.status.hasColorTemp) {
    unsigned int kelvin = (1000000 / state.color_temp);
#ifdef DEBUG
    Serial.printf("  - Got color temp: %i mired = %i Kelvin\n",
                  state.color_temp, kelvin);
#endif

    unsigned int temp = kelvin / 100;

    double red = 0;
    if (temp <= 66) {
      red = 255;
    } else {
      red = temp - 60;
      red = 329.698727446 * (pow(red, -0.1332047592));
      if (red < 0) red = 0;
      if (red > 255) red = 255;
    }

    double green = 0;
    if (temp <= 66) {
      green = temp;
      green = 99.4708025861 * log(green) - 161.1195681661;
      if (green < 0) green = 0;
      if (green > 255) green = 255;
    } else {
      green = temp - 60;
      green = 288.1221695283 * (pow(green, -0.0755148492));
      if (green < 0) green = 0;
      if (green > 255) green = 255;
    }

    double blue = 0;
    if (temp >= 66) {
      blue = 255;
    } else {
      if (temp <= 19) {
        blue = 0;
      } else {
        blue = temp - 10;
        blue = 138.5177312231 * log(blue) - 305.0447927307;
        if (blue < 0) blue = 0;
        if (blue > 255) blue = 255;
      }
    }

#ifdef DEBUG
    Serial.printf("    - RGB [%i, %i, %i]\n", static_cast<uint8_t>(red),
                  static_cast<uint8_t>(green), static_cast<uint8_t>(blue));
#endif
    state.color.r = static_cast<uint8_t>(red);
    state.color.g = static_cast<uint8_t>(green);
    state.color.b = static_cast<uint8_t>(blue);

    effects->setCurrentCommand(Effects::Command::Color);
  } else {
    // assuming turn on is the only thing.
    effects->setCurrentCommand(Effects::Command::Brightness);
  }
}

void MQTTController::handleUpdate() {
  publishInformation(
      "Got update notification. Getting ready to perform firmware update.");
#ifdef DEBUG
  Serial.println("Running ArduinoOTA");
#endif
  ArduinoOTA.begin();

  effects->setCurrentEffect(Effects::Effect::NullEffect);
  effects->setCurrentCommand(Effects::Command::FirmwareUpdate);

  effects->runCurrentCommand();
}

/**
 * Connect
 */
void MQTTController::connect() {
  IPAddress mqttip;
  WiFi.hostByName(config.mqtt_server.c_str(), mqttip);

#ifdef DEBUG
  Serial.printf("  - %s = ", config.mqtt_server.c_str());
  Serial.println(mqttip);
#endif

  while (!client.connected()) {
#ifdef DEBUG
    Serial.printf(
        "Attempting MQTT connection to \"%s\" \"%i\" as \"%s\":\"%s\" ...",
        config.mqtt_server.c_str(), config.mqtt_port,
        config.mqtt_username.c_str(), config.mqtt_password.c_str());
#endif

    if (client.connect(config.mqtt_client.c_str(), config.mqtt_username.c_str(),
                       config.mqtt_password.c_str(), statusTopic, 0, true,
                       "Disconnected")) {
#ifdef DEBUG
      Serial.println(" connected");
      Serial.printf("  - state:  '%s'\n", stateTopic);
      Serial.printf("  - command: '%s'\n", commandTopic);
#endif

      timeClient.update();

      client.subscribe(commandTopic);
      client.subscribe(queryTopic);
      client.subscribe(updateTopic);

      Serial.println("GOT HERE!");
      publishStatus();
      publishInformationData();
    } else {
#ifdef DEBUG
      Serial.print(" failed: ");
      Serial.println(client.state());
#endif
      delay(5000);
    }
  }

  Serial.println("READY TO PUBLISH STATE");
  this->_onReady();
  publishState();
}

/**
 * publish status
 * Simple function to publish if were online or not
 */
void MQTTController::publishStatus() {
  client.publish(statusTopic, "Online", true);
}

// Trying to create a global object
#endif  // IS_ESP32