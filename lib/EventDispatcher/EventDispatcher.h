#ifndef EVENTDISPATCHER_HPP
#define EVENTDISPATCHER_HPP

#include <Arduino.h>
#include <FastLED.h>

// #include <Effects.hpp>
#include <LedshelfConfig.h>
#include <LightState.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

#ifdef ESP32
#include <MQTTController.hpp>
#endif

#ifdef TEENSY
// #include <AtMQTT.h>
#include <SerialMQTTTransfer.h>
#endif

// Typedef for the lookup map for the mqtt event handlers
typedef std::map<std::string, std::function<void(std::string, std::string)>>
    TopicHandlerMap;

typedef std::function<void(LightState::LightState)> StateChangeHandler;
typedef std::function<void()> FirmwareUpdateHandler;

class EventDispatcher {
 public:
#ifdef ESP32
  MQTTController mqtt;
#elif TEENSY
  SerialMQTTTransfer mqtt;
  // AtMQTT mqtt;
#endif

  EventDispatcher();
  ~EventDispatcher(){};

  void begin();
  void loop();

  EventDispatcher& onStateChange(StateChangeHandler callback);
  EventDispatcher& onFirmwareUpdate(FirmwareUpdateHandler callback);
  EventDispatcher& onQuery();
  EventDispatcher& onError();
  EventDispatcher& onDisconnect();

  EventDispatcher& enableVerboseOutput(bool v = true);

  void setLightState(LightState::Controller& l);

  void publishInformation(const char* message);
  void publishInformation(const std::string message);
  void publishInformation();
  void publishStatus();

 private:
  TopicHandlerMap handlers;
  std::vector<StateChangeHandler> _stateHandlers;
  std::vector<FirmwareUpdateHandler> _updateHandlers;
  LedshelfConfig config;
  LightState::Controller* lightState;

  bool VERBOSE = false;

  void handleReady();
  void handleDisconnect(std::string msg);
  void handleError(std::string error);
  void handleSubscribe();
  void handleMessage(std::string topic, std::string message);
  void handleCommand(std::string topic, std::string message);
  void handleUpdate(std::string topic, std::string message);
  void handleQuery(std::string topic, std::string message);
  void handleState(std::string topic, std::string message);
  void handleStatus(std::string topic, std::string message);
  void handleInformation(std::string topic, std::string message);
};

// extern EventDispatcher EventHub;
#endif  // EVENTDISPATCHER_HPP