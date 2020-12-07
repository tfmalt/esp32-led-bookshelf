#include "EventDispatcher.h"

EventDispatcher::EventDispatcher() {
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
}

void EventDispatcher::begin() {
  if (VERBOSE) {
    mqtt.enableVerboseOutput();
  }

#ifdef TEENSY
#ifdef ATMQTT
  Serial3.begin(RXTX_BAUD_RATE);
  mqtt.begin(Serial3, config.wifi_ssid.c_str(), config.wifi_psk.c_str(),
             config.mqtt_server.c_str(), config.mqtt_port,
             config.mqtt_username.c_str(), config.mqtt_password.c_str(),
             config.mqtt_client.c_str());
#else
  mqtt.begin();
#endif
#else
  mqtt.begin();
#endif
  mqtt.onMessage(
      [this](std::string t, std::string m) { this->handleMessage(t, m); });
  mqtt.onReady([this]() { this->handleReady(); });
  mqtt.onMissingSubscribe([this]() { this->handleSubscribe(); });
  mqtt.onDisconnect([this](std::string msg) { this->handleDisconnect(msg); });
  mqtt.onError([this](std::string err) { this->handleError(err); });
}

EventDispatcher& EventDispatcher::onStateChange(StateChangeHandler callback) {
  _stateHandlers.push_back(callback);
  return *this;
}

EventDispatcher& EventDispatcher::onFirmwareUpdate(
    FirmwareUpdateHandler callback) {
  _updateHandlers.push_back(callback);
  return *this;
};

EventDispatcher& EventDispatcher::enableVerboseOutput(bool v) {
  VERBOSE = v;
  return *this;
}

void EventDispatcher::setLightState(LightState::Controller& l) {
  lightState = &l;
}

void EventDispatcher::publishInformation(const char* message) {
  mqtt.publish(config.information_topic, message);
};

void EventDispatcher::publishInformation(const std::string message) {
  publishInformation(message.c_str());
};

void EventDispatcher::publishInformation() {
  // to replace informationdata
  // mqtt.publishInformationData();
}

// ==========================================================================
// Loop
// ==========================================================================
void EventDispatcher::loop() {
  mqtt.loop();

  EVERY_N_SECONDS(60) {
    if (mqtt.connected()) {
      publishStatus();
    }
    // publishInformation();  // todo: clean up this
  }
}

void EventDispatcher::publishStatus() {
  // make this more general
  mqtt.publish(config.status_topic, "Online");
}

// ========================================================================
// Handlers for MQTT Controller events.
// ========================================================================
void EventDispatcher::handleReady() {
  Serial.println(" [hub] ||| mqtt serial is ready.");

  mqtt.publish(config.status_topic, "Online");
  // mqtt.publishInformationData();
  mqtt.publish(config.state_topic, this->lightState->getCurrentStateAsJSON());
}

void EventDispatcher::handleError(std::string error) {
  Serial.printf("[hub] Handle Error: %s\n", error.c_str());
}

void EventDispatcher::handleDisconnect(std::string msg) {
  Serial.printf("[hub] disconnect from mqtt: %s\n", msg.c_str());
}

void EventDispatcher::handleSubscribe() {
  Serial.printf(" [hub] >>> subscribe: %s\n", config.command_topic.c_str());
  mqtt.subscribe(config.command_topic);
  delay(1000);  // wait a second between each subscribe.
  Serial.printf(" [hub] >>> subscribe: %s\n", config.query_topic.c_str());
  mqtt.subscribe(config.query_topic);
  delay(1000);
  publishStatus();
}

void EventDispatcher::handleMessage(std::string topic, std::string message) {
  Serial.printf("[hub] handle message: topic: %s\n", topic.c_str());

  if (handlers.find(topic) != handlers.end()) {
    handlers[topic](topic, message);
  } else {
    Serial.println("[hub] ERROR: Got end iterator. Unknown mqtt topic.");
  }
}

void EventDispatcher::handleCommand(std::string topic, std::string message) {
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

void EventDispatcher::handleUpdate(std::string topic, std::string message) {
  Serial.printf("[hub] Handle Update: %s\n", message.c_str());

  publishInformation(
      "Got update notification. Getting ready to perform firmware update.");

  for (auto f : _updateHandlers) {
    f();
  }
}

void EventDispatcher::handleQuery(std::string topic, std::string message) {
  Serial.printf("[hub] Handle Query: %s\n", message.c_str());
}

void EventDispatcher::handleState(std::string topic, std::string message) {
  Serial.printf("[hub] Handle State: %s\n", message.c_str());
}

void EventDispatcher::handleStatus(std::string topic, std::string message) {
  Serial.printf("[hub] Handle Status: %s\n", message.c_str());
}

void EventDispatcher::handleInformation(std::string topic,
                                        std::string message) {
  Serial.printf("[hub] Handle Information: %s\n", message.c_str());
}