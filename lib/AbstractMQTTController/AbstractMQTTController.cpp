
#include "AbstractMQTTController.h"

#include <functional>
#include <string>

// ==========================================================================
// Event handler callbacks
// ==========================================================================
AbstractMQTTController& AbstractMQTTController::onMessage(
    std::function<void(std::string, std::string)> callback) {
  this->_onMessage = callback;
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onReady(
    std::function<void()> callback) {
  this->_onReady = callback;
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onDisconnect(
    std::function<void(std::string msg)> callback) {
  this->_onDisconnect = callback;
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onError(
    std::function<void(std::string error)> callback) {
  this->_onError = callback;
  return *this;
}

AbstractMQTTController& AbstractMQTTController::enableVerboseOutput(
    bool v = true) {
  this->_verbose = v;
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitReady() {
  this->_onReady();
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitDisconnect(std::string m) {
  this->_onDisconnect(m);
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitError(std::string e) {
  this->_onError(e);
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitMessage(std::string t,
                                                            std::string m) {
  this->_onMessage(t, m);
  return *this;
}