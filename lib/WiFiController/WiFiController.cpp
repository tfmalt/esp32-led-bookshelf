#ifdef ESP32
#include "WiFiController.hpp"
// #include <WiFiClientSecure.h>
#include <WiFi.h>

void WiFiController::setup() {
  WiFi.onEvent(
      [this](WiFiEvent_t event, WiFiEventInfo_t info) { handleEvent(event); });
}

/**
 * Configure and setup wifi
 */
void WiFiController::connect() {
#ifdef DEBUG
  Serial.printf("[wifi] Connecting to: %s:%s\n", config.wifi_ssid.c_str(),
                config.wifi_psk.c_str());
#endif

  WiFi.begin(config.wifi_ssid.c_str(), config.wifi_psk.c_str());
  WiFi.setHostname(config.wifi_hostname.c_str());

  // Wait here until we are connected.
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
};

// WiFiClientSecure &WiFiController::getWiFiClient()
WiFiClient& WiFiController::getWiFiClient() {
  return wifiClient;
};

void WiFiController::testOutput() {
#ifdef DEBUG
  Serial.printf("[wifi]   Testing output.\n");
  Serial.println(config.wifi_ssid.c_str());
#endif
}

void WiFiController::handleEvent(WiFiEvent_t event) {
  switch (event) {
    // SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
    case SYSTEM_EVENT_WIFI_READY:
#ifdef DEBUG
      Serial.printf("[wifi]   WiFi Ready [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
    case SYSTEM_EVENT_SCAN_DONE:
#ifdef DEBUG
      Serial.printf("[wifi]   got SYSTEM_EVENT_SCAN_DONE [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_START                < ESP32 station start
    case SYSTEM_EVENT_STA_START:
#ifdef DEBUG
      Serial.printf("[wifi]   Starting [%i] ...\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
    case SYSTEM_EVENT_STA_STOP:
#ifdef DEBUG
      Serial.printf("[wifi]   got SYSTEM_EVENT_STA_STOP [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
    case SYSTEM_EVENT_STA_CONNECTED:
#ifdef DEBUG
      Serial.printf("[wifi]   Connected to access point \"%s\" [%i]\n",
                    config.wifi_ssid.c_str(), event);
#endif
      break;
    // SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from
    // AP
    case SYSTEM_EVENT_STA_DISCONNECTED:
#ifdef DEBUG
      Serial.printf("[wifi]   got SYSTEM_EVENT_STA_DISCONNECTED [%i]\n", event);
      Serial.println("[wifi] restarting...");
#endif
      // sleeping for half a second to let things settle
      delay(500);
      ESP.restart();
      break;
    // SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by
    // ESP32 station changed
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
#ifdef DEBUG
      Serial.printf("[wifi]   got SYSTEM_EVENT_STA_AUTHMODE_CHANGE [%i]\n",
                    event);
#endif
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
#ifdef DEBUG
      Serial.printf("[wifi]   Got IP address [%i]\n", event);
      Serial.print("[wifi]      IP:  ");
      Serial.println(WiFi.localIP());
      Serial.print("[wifi]      DNS: ");
      Serial.println(WiFi.dnsIP());
#endif
      break;
    default:
#ifdef DEBUG
      Serial.printf("[wifi] Got other event: [%i]\n", event);
#endif
      break;
  }
}

#endif  // ESP32