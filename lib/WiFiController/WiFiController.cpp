#ifdef IS_ESP32
#include "WiFiController.h"
// #include <WiFiClientSecure.h>
#include <LedshelfConfig.h>
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
  Serial.printf("  - Connecting to: %s:%s\n", config.wifi_ssid,
                config.wifi_psk);
#endif

  WiFi.begin(config.wifi_ssid, config.wifi_psk);
  WiFi.setHostname(config.wifi_hostname);

  // Wait here until we are connected.
  while (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
    Serial.print(".");
#endif
    delay(1000);
  }
#ifdef DEBUG
  Serial.println("");
#endif

  // wifiClient.setCACert(config.ca_root);
};

// WiFiClientSecure &WiFiController::getWiFiClient()
WiFiClient &WiFiController::getWiFiClient() { return wifiClient; };

void WiFiController::testOutput() {
#ifdef DEBUG
  Serial.printf("    - Testing output.\n");
  Serial.println(config.wifi_ssid);
#endif
}

void WiFiController::handleEvent(WiFiEvent_t event) {
  // SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is
  // reset to 0 SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps
  // succeeds in enrollee mode SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32
  // station wps fails in enrollee mode SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       <
  // ESP32 station wps timeout in enrollee mode SYSTEM_EVENT_STA_WPS_ER_PIN <
  // ESP32 station wps pin code in enrollee mode SYSTEM_EVENT_AP_START < ESP32
  // soft-AP start SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
  // SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32
  // soft-AP SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from
  // ESP32 soft-AP SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request
  // packet in soft-AP interface SYSTEM_EVENT_ETH_START                < ESP32
  // ethernet start SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
  // SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
  // SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
  // SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from
  // connected AP SYSTEM_EVENT_MAX
  // */

  switch (event) {
    // SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
    case SYSTEM_EVENT_WIFI_READY:
#ifdef DEBUG
      Serial.printf("  - WiFi Ready [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
    case SYSTEM_EVENT_SCAN_DONE:
#ifdef DEBUG
      Serial.printf("WIFI: got SYSTEM_EVENT_SCAN_DONE [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_START                < ESP32 station start
    case SYSTEM_EVENT_STA_START:
#ifdef DEBUG
      Serial.printf("  - Starting [%i] ...\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
    case SYSTEM_EVENT_STA_STOP:
#ifdef DEBUG
      Serial.printf("WIFI: got SYSTEM_EVENT_STA_STOP [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
    case SYSTEM_EVENT_STA_CONNECTED:
#ifdef DEBUG
      Serial.printf("  - Connected to access point [%s][%i]\n",
                    config.wifi_ssid, event);
#endif
      break;
    // SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from
    // AP
    case SYSTEM_EVENT_STA_DISCONNECTED:
#ifdef DEBUG
      Serial.printf("WIFI: got SYSTEM_EVENT_STA_DISCONNECTED [%i]\n", event);
      Serial.println("  - restarting...");
#endif
      // sleeping for half a second to let things settle
      delay(500);
      ESP.restart();
      break;
    // SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by
    // ESP32 station changed
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
#ifdef DEBUG
      Serial.printf("WIFI: got SYSTEM_EVENT_STA_AUTHMODE_CHANGE [%i]\n", event);
#endif
      break;
    // SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from
    // connected AP
    case SYSTEM_EVENT_STA_GOT_IP:
#ifdef DEBUG
      Serial.printf("  - Got IP address [%i]\n", event);
      Serial.print("    - IP:  ");
      Serial.println(WiFi.localIP());
      Serial.print("    - DNS: ");
      Serial.println(WiFi.dnsIP());
#endif
      break;
    default:
#ifdef DEBUG
      Serial.printf("WIFI: Got other event: [%i]\n", event);
#endif
      break;
  }
}

#endif  // IS_ESP32