
#include "AbstractMQTTController.h"

#include <functional>
#include <string>

// ==========================================================================
// Event handler callbacks
// ==========================================================================

AbstractMQTTController& AbstractMQTTController::onMessage(
    OnMessageFunction _c) {
  this->_onMessageList.push_back(_c);
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onReady(OnReadyFunction _c) {
  this->_onReadyList.push_back(_c);
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onDisconnect(
    OnDisconnectFunction _c) {
  this->_onDisconnectList.push_back(_c);
  return *this;
}

AbstractMQTTController& AbstractMQTTController::onError(OnErrorFunction _c) {
  this->_onErrorList.push_back(_c);
  return *this;
}

// AbstractMQTTController& AbstractMQTTController::onMessage(
//     std::function<void(std::string, std::string)> callback) {
//   this->_onMessage = callback;
//   return *this;
// }
//
// AbstractMQTTController& AbstractMQTTController::onReady(
//     std::function<void()> callback) {
//   this->_onReady = callback;
//   return *this;
// }
//
// AbstractMQTTController& AbstractMQTTController::onDisconnect(
//     std::function<void(std::string msg)> callback) {
//   this->_onDisconnect = callback;
//   return *this;
// }
//
// AbstractMQTTController& AbstractMQTTController::onError(
//     std::function<void(std::string error)> callback) {
//   this->_onError = callback;
//   return *this;
// }

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

AbstractMQTTController& AbstractMQTTController::enableVerboseOutput() {
  return enableVerboseOutput(true);
};

AbstractMQTTController& AbstractMQTTController::enableVerboseOutput(
    bool v = true) {
  this->_verbose = v;
  return *this;
}

bool AbstractMQTTController::verbose() {
  return _verbose;
};
