#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>

#include <Effects.hpp>
#include <LedshelfConfig.hpp>
#include <LightStateController.hpp>
#include <map>
#include <string>

#ifdef IS_ESP32
#include <MQTTController.hpp>
#endif  // IS_ESP32

#ifdef IS_TEENSY
#include <SerialMQTT.hpp>
#endif

// Typedef for the lookup map for the mqtt event handlers
typedef std::map<std::string, std::function<void(std::string, std::string)>>
    TopicHandlerMap;

class EventDispatcher {
 public:
  EventDispatcher() {
    handlers[config.state_topic] = [this](std::string topic,
                                          std::string message) {
      this->handleState(topic, message);
    };
    handlers[config.command_topic] = [this](std::string topic,
                                            std::string message) {
      this->handleCommand(topic, message);
    };
    handlers[config.status_topic] = [this](std::string topic,
                                           std::string message) {
      this->handleStatus(topic, message);
    };
    handlers[config.query_topic] = [this](std::string topic,
                                          std::string message) {
      this->handleQuery(topic, message);
    };
    handlers[config.information_topic] = [this](std::string topic,
                                                std::string message) {
      this->handleInformation(topic, message);
    };
    handlers[config.update_topic] = [this](std::string topic,
                                           std::string message) {
      this->handleUpdate(topic, message);
    };
  };

  void setup() {
    mqtt.setup();
    mqtt.onMessage(
        [this](std::string t, std::string m) { this->handleMessage(t, m); });
    mqtt.onReady([this]() { this->handleReady(); });
    mqtt.onDisconnect([this](std::string msg) { this->handleDisconnect(msg); });
    mqtt.onError([this](std::string err) { this->handleError(err); });
  }

  void setEffects(Effects *e) { effects = e; }
  void setLightState(LightStateController *l) { lightState = l; }

  void publishInformation(const char *message) {
    mqtt.publish(config.information_topic, message);
  };

  void publishInformation(const std::string message) {
    publishInformation(message.c_str());
  };

  void publishInformation() {
    // to replace informationdata
    mqtt.publishInformationData();
  }

  void publishStatus() {
    // make this more general
    mqtt.publish(config.status_topic, "Online");
  }

  void loop() {
    mqtt.checkConnection();

    EVERY_N_SECONDS(60) {
      publishStatus();
      publishInformation();  // todo: clean up this
    }
  }

 private:
  TopicHandlerMap handlers;
  LedshelfConfig config;
  Effects *effects;
  LightStateController *lightState;

#ifdef IS_ESP32
  MQTTController mqtt;
#endif
#ifdef IS_TEENSY
  SerialMQTT mqtt;
#endif

  bool isFirmwareUpdateActive() {
    return (effects->currentCommandType == Effects::Command::FirmwareUpdate);
  }

  void handleMessage(std::string topic, std::string message) {
    Serial.printf("  - Handle Message: topic: %s\n", topic.c_str());

    if (isFirmwareUpdateActive()) {
      Serial.println("    - Firmware update active. Ignoring command.");
      publishInformation("Firmware update active. Ignoring command.");
      return;
    }

    if (handlers.find(topic) != handlers.end()) {
      handlers[topic](topic, message);
    } else {
      Serial.println("Error: Got end iterator. Unknown mqtt topic.");
    }
  }

  void handleReady() {
    // got ready handler
    Serial.println("[hub] Got MQTT Ready");

    mqtt.publish(config.status_topic, "Online");
    mqtt.publishInformationData();
    mqtt.publish(config.state_topic, this->lightState->getCurrentStateAsJSON());
  }

  void handleDisconnect(std::string msg) {
    Serial.printf("[hub] Disconnect: %s\n", msg.c_str());
  }

  void handleError(std::string error) {
    Serial.printf("[hub] Handle Error: %s\n", error.c_str());
  }

  void handleState(std::string topic, std::string message) {
    Serial.printf("Handle State: %s\n", message.c_str());
  }

  void handleCommand(std::string topic, std::string message) {
    Serial.printf("Handle Command: %s\n", message.c_str());

    if (this->lightState == nullptr) {
      Serial.println("  ERROR: Lightstate not set. got nullptr.");
      return;
    }

    this->lightState->parseNewState(message);
    _handleStateChange();
    std::string json = this->lightState->getCurrentStateAsJSON();
    mqtt.publish(config.state_topic, json);
  }

  /**
   * Handle the actual state update
   */
  void _handleStateChange() {
    LightState &state = this->lightState->getCurrentState();

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

  void handleStatus(std::string topic, std::string message) {
    Serial.printf("Handle Status: %s\n", message.c_str());
  }

  void handleQuery(std::string topic, std::string message) {
    Serial.printf("Handle Query: %s\n", message.c_str());
  }

  void handleInformation(std::string topic, std::string message) {
    Serial.printf("Handle Information: %s\n", message.c_str());
  }

  void handleUpdate(std::string topic, std::string message) {
    Serial.printf("Handle Update: %s\n", message.c_str());

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
};

#endif  // EVENTDISPATCHER_HPP