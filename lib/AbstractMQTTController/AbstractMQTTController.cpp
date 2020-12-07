
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

AbstractMQTTController& AbstractMQTTController::emitReady() {
  Serial.println("Inside emitReady");
  for (auto func : this->_onReadyList) {
    if (func != nullptr) {
      func();
    }
  }

  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitDisconnect(std::string m) {
  for (auto func : this->_onDisconnectList) {
    if (func != nullptr) {
      func(m);
    }
  }
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitError(std::string e) {
  for (auto func : this->_onErrorList) {
    if (func != nullptr) {
      func(e);
    }
  }
  return *this;
}

AbstractMQTTController& AbstractMQTTController::emitMessage(std::string t,
                                                            std::string m) {
  for (auto func : this->_onMessageList) {
    if (func != nullptr) {
      func(t, m);
    }
  }
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
