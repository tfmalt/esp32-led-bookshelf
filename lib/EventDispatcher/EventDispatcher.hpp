#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <Arduino.h>
#include <FastLED.h>

// #include <Effects.hpp>
#include <LedshelfConfig.hpp>
#include <LightState.hpp>
#include <map>
#include <string>
#include <vector>

#ifdef IS_ESP32
#include <MQTTController.hpp>
#endif  // IS_ESP32

#ifdef IS_TEENSY
#include <SerialMQTT.hpp>
#endif

// Typedef for the lookup map for the mqtt event handlers
typedef std::map<std::string, std::function<void(std::string, std::string)>>
    TopicHandlerMap;

typedef std::function<void(LightState::LightState)> StateChangeHandler;
typedef std::function<void()> FirmwareUpdateHandler;

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

  ~EventDispatcher(){};

  void setup() {
    mqtt.setup();
    mqtt.onMessage(
        [this](std::string t, std::string m) { this->handleMessage(t, m); });
    mqtt.onReady([this]() { this->handleReady(); });
    mqtt.onDisconnect([this](std::string msg) { this->handleDisconnect(msg); });
    mqtt.onError([this](std::string err) { this->handleError(err); });
  }

  EventDispatcher &onStateChange(StateChangeHandler callback) {
    _stateHandlers.push_back(callback);
    return *this;
  };
  EventDispatcher &onFirmwareUpdate(FirmwareUpdateHandler callback) {
    _updateHandlers.push_back(callback);
    return *this;
  };

  EventDispatcher &onQuery();
  EventDispatcher &onError();
  EventDispatcher &onDisconnect();

  // void setEffects(Effects::Controller *e) { effects = e; }
  void setLightState(LightState::Controller *l) { lightState = l; }

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
    mqtt.loop();

    EVERY_N_SECONDS(60) {
      publishStatus();
      publishInformation();  // todo: clean up this
    }
  }

 private:
  TopicHandlerMap handlers;
  std::vector<StateChangeHandler> _stateHandlers;
  std::vector<FirmwareUpdateHandler> _updateHandlers;
  LedshelfConfig config;
  // Effects::Controller *effects;
  LightState::Controller *lightState;

#ifdef IS_ESP32
  MQTTController mqtt;
#endif
#ifdef IS_TEENSY
  SerialMQTT mqtt;
#endif

  // bool isFirmwareUpdateActive() {
  //   return (effects->currentCommandType == Effects::Command::FirmwareUpdate);
  // }

  void handleMessage(std::string topic, std::string message) {
    Serial.printf("[hub] handle message: topic: %s\n", topic.c_str());

    // if (isFirmwareUpdateActive()) {
    //   Serial.println("[hub]   Firmware update active. Ignoring command.");
    //   publishInformation("Firmware update active. Ignoring command.");
    //   return;
    // }

    if (handlers.find(topic) != handlers.end()) {
      handlers[topic](topic, message);
    } else {
      Serial.println("[hub] ERROR: Got end iterator. Unknown mqtt topic.");
    }
  }

  // ========================================================================
  // Handlers for MQTT Controller events.
  // ========================================================================
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

  // ========================================================================
  // Handlers for MQTT Topic events.
  // ========================================================================
  void handleCommand(std::string topic, std::string message) {
    Serial.printf("[hub] handle command: %s\n", message.c_str());

    if (this->lightState == nullptr) {
      Serial.println("[hub] ERROR: Lightstate not set. got nullptr.");
      return;
    }

    LightState::LightState state = this->lightState->parseNewState(message);

    for (auto callback : _stateHandlers) {
      callback(state);
    }

    std::string json = this->lightState->getCurrentStateAsJSON();
    mqtt.publish(config.state_topic, json);
  }

  void handleUpdate(std::string topic, std::string message) {
    Serial.printf("[hub] Handle Update: %s\n", message.c_str());

    publishInformation(
        "Got update notification. Getting ready to perform firmware update.");
    //  #ifdef DEBUG
    //      Serial.println("Running ArduinoOTA");
    //  #endif

    for (auto f : _updateHandlers) {
      f();
    }
  }

  void handleQuery(std::string topic, std::string message) {
    Serial.printf("[hub] Handle Query: %s\n", message.c_str());
  }

  void handleState(std::string topic, std::string message) {
    Serial.printf("[hub] Handle State: %s\n", message.c_str());
  }

  void handleStatus(std::string topic, std::string message) {
    Serial.printf("[hub] Handle Status: %s\n", message.c_str());
  }

  void handleInformation(std::string topic, std::string message) {
    Serial.printf("[hub] Handle Information: %s\n", message.c_str());
  }
};

extern EventDispatcher EventHub;
#endif  // EVENTDISPATCHER_HPP