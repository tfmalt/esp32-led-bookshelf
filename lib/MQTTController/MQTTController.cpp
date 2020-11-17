
#include "MQTTController.hpp"

#include <NTPClient.h>
#include <WiFiUdp.h>

#include <functional>
#include <string>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

MQTTController::MQTTController(const char* _wifi_ssid,
                               const char* _wifi_psk,
                               const char* _wifi_hostname,
                               const char* _mqtt_server,
                               const char* _mqtt_port,
                               const char* _mqtt_client) {}
/**
 * Setup this instance of controller
 */
MQTTController& MQTTController::setup() {
#ifdef DEBUG
  Serial.printf("[mqtt] Setting up client: %s:%i\n", config.mqtt_server.c_str(),
                config.mqtt_port);
#endif

  wifiCtrl.setup();
  wifiCtrl.connect();

  timeClient.begin();

  WiFiClient& wifi = wifiCtrl.getWiFiClient();
#ifdef DEBUG
  Serial.println("[mqtt] Got wifi client");
#endif

#ifdef DEBUG
  Serial.printf("[mqtt] NTP time client started: %s.\n",
                timeClient.getFormattedTime().c_str());
#endif

  bool setbuf = client.setBufferSize(512);

  Serial.printf("[mqtt] Set MQTT buffer to 512: %s\n",
                (setbuf) ? "true" : "false");

  client.setClient(wifi);
  client.setServer(config.mqtt_server.c_str(), config.mqtt_port);
  client.setCallback(
      [this](char* p_topic, byte* p_message, unsigned int p_length) {
        std::string topic = p_topic;
        std::string message(reinterpret_cast<const char*>(p_message), p_length);

        _onMessage(topic, message);
      });

  Serial.println("[mqtt] Setup Finished.");
  return *this;
}

/**
 * Connect
 */
void MQTTController::connect() {
  while (!client.connected()) {
#ifdef DEBUG
    Serial.printf("[mqtt] Attempting connection to %s:%i as \"%s\" ...",
                  config.mqtt_server.c_str(), config.mqtt_port,
                  config.mqtt_username.c_str());
#endif

    if (client.connect(config.mqtt_client.c_str(), config.mqtt_username.c_str(),
                       config.mqtt_password.c_str(),
                       config.status_topic.c_str(), 0, true, "Disconnected")) {
      timeClient.update();

      client.subscribe(config.command_topic.c_str());
      client.subscribe(config.query_topic.c_str());
      client.subscribe(config.update_topic.c_str());

    } else {
#ifdef DEBUG
      Serial.print(" failed: ");
      Serial.println(client.state());
#endif
      delay(5000);
    }
  }

  Serial.println(" Connected.");

  this->_onReady();
}

// ==========================================================================
// Event handler callbacks
// ==========================================================================
MQTTController& MQTTController::onMessage(
    std::function<void(std::string, std::string)> callback) {
  this->_onMessage = callback;
  return *this;
}

MQTTController& MQTTController::onReady(std::function<void()> callback) {
  this->_onReady = callback;
  return *this;
}

MQTTController& MQTTController::onDisconnect(
    std::function<void(std::string msg)> callback) {
  this->_onDisconnect = callback;
  return *this;
}

MQTTController& MQTTController::onError(
    std::function<void(std::string error)> callback) {
  this->_onError = callback;
  return *this;
}
// ==========================================================================
// End of Event handler callbacks
// ==========================================================================

void MQTTController::loop() {
  if (WiFi.status() != WL_CONNECTED) {
    this->_onError("Restarting because WiFi not connected.");
    delay(1000);
    ESP.restart();
    return;
  }
  if (!client.connected()) {
    std::string msg = "MQTT broker not connected: " + config.mqtt_server;
    this->_onDisconnect(msg);
    connect();
  }

  client.loop();
}

bool MQTTController::publish(const char* topic, const char* message) {
  // TODO: Add assertion that topic is corret.
  return client.publish(topic, message, false);
}

bool MQTTController::publish(std::string topic, std::string message) {
  return publish(topic.c_str(), message.c_str());
}

/**
 * publish information data
 * compile some useful metainformation about the system
 */
void MQTTController::publishInformationData() {
  char* msg;

  WiFiClient& wifi = wifiCtrl.getWiFiClient();

  asprintf(&msg,
           "{\"time\": \"%s\", \"hostname\": \"%s\", \"version\": \"%s\", "
           "\"uptime\": %lu, \"memory\": %d }",

           timeClient.getFormattedTime().c_str(), WiFi.getHostname(), version,
           millis(), xPortGetFreeHeapSize());

  const char* message = msg;
  client.publish(config.information_topic.c_str(), message, false);

  free(msg);
}
