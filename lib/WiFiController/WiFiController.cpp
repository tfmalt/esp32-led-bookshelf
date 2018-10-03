#include "WiFiController.h"
#include <WiFiClientSecure.h>


void WiFiController::setup(String ssid_i, String psk_i)
{
    ssid_p = ssid_i;
    psk_p  = psk_i;

    WiFi.onEvent([this](WiFiEvent_t event, WiFiEventInfo_t info) {
        handleEvent(event);
    });
}

/**
 * Configure and setup wifi
 */
void WiFiController::connect()
{
    Serial.printf("Connecting to: %s:\n", ssid.c_str());

    WiFi.begin(ssid.c_str(), psk.c_str());

    // Wait here until we are connected.
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
};

WiFiClientSecure& WiFiController::getWiFiClient() {
    return wifiClient;
};

void WiFiController::testOutput()
{
    Serial.printf("    - Testing output.\n");
    Serial.println(ssid);
}

void WiFiController::handleEvent(WiFiEvent_t event)
{
    // SYSTEM_EVENT_STA_LOST_IP              < ESP32 station lost IP and the IP is reset to 0
    // SYSTEM_EVENT_STA_WPS_ER_SUCCESS       < ESP32 station wps succeeds in enrollee mode
    // SYSTEM_EVENT_STA_WPS_ER_FAILED        < ESP32 station wps fails in enrollee mode
    // SYSTEM_EVENT_STA_WPS_ER_TIMEOUT       < ESP32 station wps timeout in enrollee mode
    // SYSTEM_EVENT_STA_WPS_ER_PIN           < ESP32 station wps pin code in enrollee mode
    // SYSTEM_EVENT_AP_START                 < ESP32 soft-AP start
    // SYSTEM_EVENT_AP_STOP                  < ESP32 soft-AP stop
    // SYSTEM_EVENT_AP_STACONNECTED          < a station connected to ESP32 soft-AP
    // SYSTEM_EVENT_AP_STADISCONNECTED       < a station disconnected from ESP32 soft-AP
    // SYSTEM_EVENT_AP_PROBEREQRECVED        < Receive probe request packet in soft-AP interface
    // SYSTEM_EVENT_ETH_START                < ESP32 ethernet start
    // SYSTEM_EVENT_ETH_STOP                 < ESP32 ethernet stop
    // SYSTEM_EVENT_ETH_CONNECTED            < ESP32 ethernet phy link up
    // SYSTEM_EVENT_ETH_DISCONNECTED         < ESP32 ethernet phy link down
    // SYSTEM_EVENT_ETH_GOT_IP               < ESP32 ethernet got IP from connected AP
    // SYSTEM_EVENT_MAX
    // */

    switch (event)
    {
        // SYSTEM_EVENT_WIFI_READY               < ESP32 WiFi ready
        case SYSTEM_EVENT_WIFI_READY :
            Serial.printf("  - WiFi Ready [%i]\n", event);
            break;
        // SYSTEM_EVENT_SCAN_DONE                < ESP32 finish scanning AP
        case SYSTEM_EVENT_SCAN_DONE :
            Serial.printf("WIFI: got SYSTEM_EVENT_SCAN_DONE [%i]\n", event);
            break;
        // SYSTEM_EVENT_STA_START                < ESP32 station start
        case SYSTEM_EVENT_STA_START :
            Serial.printf("  - Starting ... [%i]\n", event);
            break;
        // SYSTEM_EVENT_STA_STOP                 < ESP32 station stop
        case SYSTEM_EVENT_STA_STOP :
            Serial.printf("WIFI: got SYSTEM_EVENT_STA_STOP [%i]\n", event);
            break;
        // SYSTEM_EVENT_STA_CONNECTED            < ESP32 station connected to AP
        case SYSTEM_EVENT_STA_CONNECTED :
            WiFi.enableIpV6();
            Serial.printf("  - Connected to access point [%s][%i]\n", ssid.c_str(), event);
            // testOutput();
            break;
        // SYSTEM_EVENT_STA_DISCONNECTED         < ESP32 station disconnected from AP
        case SYSTEM_EVENT_STA_DISCONNECTED :
            Serial.printf("WIFI: got SYSTEM_EVENT_STA_DISCONNECTED [%i]\n", event);
            Serial.println("  - implement wifi reconnect here.");
            // sleeping for half a second to let things settle
            delay(500);
            break;
        // SYSTEM_EVENT_STA_AUTHMODE_CHANGE      < the auth mode of AP connected by ESP32 station changed
        case SYSTEM_EVENT_STA_AUTHMODE_CHANGE :
            Serial.printf("WIFI: got SYSTEM_EVENT_STA_AUTHMODE_CHANGE [%i]\n", event);
            break;
        // SYSTEM_EVENT_STA_GOT_IP               < ESP32 station got IP from connected AP
        case SYSTEM_EVENT_STA_GOT_IP :
            Serial.printf("  - Got IP address [%i]\n", event);
            Serial.print("    - IP:  ");
            Serial.println(WiFi.localIP());
            Serial.print("    - DNS: ");
            Serial.println(WiFi.dnsIP());
            break;
        // SYSTEM_EVENT_GOT_IP6                  < ESP32 station or ap or ethernet interface v6IP addr is preferred
        case SYSTEM_EVENT_GOT_IP6 :
            //both interfaces get the same event
            Serial.printf("  - Got IPv6 address: [%i]\n", event);
            Serial.print("    - STA IPv6: ");
            Serial.println(WiFi.localIPv6());
            break;
        default :
            Serial.printf("WIFI: Got other event: [%i]\n", event);
            break;
    }
}