/**
 * @file AtMQTT.h
 * @author Thomas Malt <thomas@malt.no>
 * @brief Library for connecting to the latest AT firmware from espressif,
 * esp32 and esp8266
 * @version 0.9
 * @date 2020-11-28
 *
 * @copyright Copyright (c) 2020
 *
 */

#ifndef ATMQTT_H
#define ATMQTT_H
#include <AbstractMQTTController.h>
#include <Arduino.h>
#include <IPAddress.h>
#include <stdlib.h>
#include <functional>
#include <map>
#include <string>
#include <vector>

// ==========================================================================
// Auxillary classes. Implement support for the AT return types.
// ==========================================================================

class GMRInfo {
 public:
  std::string atVersion;
  std::string sdkVersion;
  std::string binVersion;
  std::string compileTime;
};

enum CIPStatus {
  INACTIVE = 0,
  IDLE,
  AP_CONNECTED,
  CONNECTION_CREATED,
  CONNECTION_DISCONNECTED,
  AP_DISCONNECTED
};

class ATResult {
 public:
  bool successful;
  std::string status;
  std::string body;
};

enum MQTTState {
  UNINITIALIZED = 0,
  SET_BY_MQTTUSERCFG,
  SET_BY_MQTTCONNCFG,
  DISCONNECTED,
  ESTABLISHED,
  CONNECTED_NO_TOPIC,
  CONNECTED_TOPIC
};

enum MQTTScheme {
  TCP = 1,
  TLS,
  TLS_VERIFY,
  TLS_PROVIDE,
  TLS_VERIFY_PROVIDE,
  WS,
  WS_TLS,
  WS_TLS_VERIFY,
  WS_TLS_PROVIDE,
  WS_TLS_VERIFY_PROVIDE
};

class MQTTConnectionStatus {
 public:
  uint8_t linkId;
  MQTTState state;
  MQTTScheme scheme;
  std::string hostname;
  uint16_t port;
  std::string path;
  bool reconnect;

  std::string toString();
};

constexpr uint16_t WIFI_RECONN_INTERVAL = 1;
constexpr uint16_t WIFI_LISTEN_INTERVAL = 3;

/**
 * @brief An implementation for connecting to MQTT over AT.
 *
 */
class AtMQTT : public AbstractMQTTController {
 public:
  typedef std::vector<std::string> TokenList;
  typedef std::function<void()> OnIntervalHandler;
  typedef std::vector<OnIntervalHandler> OnIntervalHandlerList;
  typedef std::map<uint32_t, OnIntervalHandlerList> OnIntervalHandlerMap;
  typedef std::map<uint32_t, uint32_t> OnIntervalTimerList;

  OnIntervalHandlerMap _intervalHandlers;
  OnIntervalTimerList _intervalTimers;

  AtMQTT() {}
  ~AtMQTT() {}

  static std::string itos(int number);
  static TokenList ssplit(std::string text, std::string separator);

  void begin() {
    if (atSerial == nullptr) {
      Serial.println(
          "[mqtt] ERROR: AtMQTT driver requires reference to Serial");
      return;
    }

    if (verbose()) {
      Serial.println("[mqtt] Setting up AtMQTT driver");
    }

    //     this->onIntervalMillis(
    //         2000, [this]() { Serial.println("[mqtt]  first beacon. In loop");
    //         });
    //     this->onIntervalMillis(
    //         2500, [this]() { Serial.println("[mqtt]  second beacon. In
    //         loop"); });
    //     this->onIntervalMillis(
    //         2000, [this]() { Serial.println("[mqtt]  third beacon. In loop");
    //         });

    while (!this->connect()) {
      this->emitError("Could not connect correctly");
      delay(5000);
    }

    this->emitReady();
  }

  void begin(Stream& _at) {
    atSerial = &_at;

    this->begin();
  }

  void begin(Stream& _at, const char* _ssid, const char* _psk) {
    ssid = (char*)_ssid;
    psk = (char*)_psk;

    this->begin(_at);
  }

  void begin(Stream& _at,
             const char* _ssid,
             const char* _psk,
             const char* _server,
             uint16_t _port = 1883,
             const char* _user = "",
             const char* _pass = "",
             const char* _client = "") {
    server = (char*)_server;
    port = _port;
    username = (char*)_user;
    password = (char*)_pass;
    client = (char*)_client;

    this->begin(_at, _ssid, _psk);
  }

  /**
   * @brief Orchestrate to
   *
   * @return true
   * @return false
   */
  bool connect() {
    if (!this->connectSerial()) {
      return false;
    }

    Serial.printf("[mqtt] %-40s", "Resetting wifi+mqtt device: ");
    bool res = this->restoreFactoryDefault();
    Serial.printf("[%s]\n", res ? "ok" : "fail");
    delay(500);
    this->disableEcho();

    if (!this->connectWifi()) {
      return false;
    }

    if (!this->connectMQTT()) {
      return false;
    };

    return true;
  }

  bool connectSerial() {
    Serial.printf("[mqtt] %-40s", "  Configuring serial connection ... ");
    if (!this->disableEcho()) {
      Serial.println("[fail]");
      Serial.println("  ERROR: AtMQTT Serial not connected.");
      return false;
    }
    Serial.println("[ok]");

    GMRInfo gmr = this->getGMRInfo();
    if (verbose()) {
      if (gmr.atVersion.length() > 0) {
        Serial.printf("[mqtt]   got version info: '%s'\n",
                      gmr.atVersion.c_str());
      } else {
        Serial.println("[mqtt]   ERROR: did not get version info.");
      }
    }

    if (gmr.atVersion.length() == 0) {
      return false;
    }

    if (verbose()) {
      Serial.println("[mqtt]   AtMQTT Connection success");
    }

    return true;
  }

  bool connectWifi() {
    ATResult res;

    if (verbose()) {
      Serial.println("[mqtt] Starting WiFi ...");
    }

    CIPStatus status = this->getCIPStatus();
    // res = this->execute("AT+CWJAP?");
    Serial.printf("[mqtt]   cipstatus: [%i]\n", status);
    if ((int)status >= (int)CIPStatus::AP_CONNECTED &&
        (int)status < (int)CIPStatus::AP_DISCONNECTED) {
      Serial.println("[mqtt]   connected");
      return true;
    }
    // set station mode
    res = this->execute("AT+CWMODE=1");
    if (verbose()) {
      Serial.printf("[mqtt] %-40s[%s]\n",
                    "setting station mode:", res.successful ? "ok" : "fail");
    }
    if (!res.successful) {
      return false;
    }

    // connect to access point
    std::string cwjap;
    cwjap = "AT+CWJAP=\"";
    cwjap += ssid + "\",\"";
    cwjap += psk + "\",,,";
    cwjap += itos(WIFI_RECONN_INTERVAL) + "," + itos(WIFI_LISTEN_INTERVAL);

    res = this->execute(cwjap);

    if (verbose()) {
      Serial.printf("[mqtt]   ap connect: [%s]\n",
                    res.successful ? "ok" : "fail");
      if (res.successful) {
        Serial.printf("[mqtt]   ip address: %s\n", getIPAddress().c_str());
      }
    }

    return res.successful;
  }

  bool closeMQTT() {
    // AT+MQTTCLEAN=<LinkID>
    ATResult res = this->execute("AT+MQTTCLEAN");
    if (verbose()) {
      Serial.printf("[mqtt]   close connection: %s\n",
                    res.successful ? "true" : "false");
      Serial.printf("[mqtt]     [%s] '%s'\n", res.status.c_str(),
                    res.body.c_str());
    }
    return res.successful;
  }

  /**
   * @brief Orchestrates the connection to a MQTT broker
   *
   * @return true
   * @return false
   */
  bool connectMQTT() {
    std::string cfg;
    std::string conn;

    cfg = "AT+MQTTUSERCFG=0,1,";
    cfg += "\"" + this->client + "\",";
    cfg += "\"" + this->username + "\",";
    cfg += "\"" + this->password + "\",0,0,\"\"";

    conn = "AT+MQTTCONN=0,";
    conn += "\"" + this->server + "\",";
    conn += itos(this->port) + ",1";

    ATResult res;

    if (verbose()) {
      Serial.println("[mqtt] Starting MQTT ...");
    }

    MQTTConnectionStatus mqs;
    Serial.printf("[mqtt] %s", "  setting configuration: ...");

    // set configuration
    do {
      res = this->execute(cfg);
      Serial.print(".");
      delay(1000);
      mqs = this->_getMQTTConnectionStatus();
    } while ((int)mqs.state < (int)MQTTState::SET_BY_MQTTUSERCFG);

    Serial.printf("[%s]\n", res.successful ? "ok" : "fail");

    if (mqs.state >= MQTTState::ESTABLISHED) {
      Serial.printf("[mqtt]   already connected [%i]\n", mqs.state);
      return true;
    }

    Serial.printf("[mqtt]   connecting [%i] ", mqs.state);
    Serial.println(conn.c_str());

    // connect to broker
    do {
      delay(1000);
      res = this->execute(conn);
      Serial.printf("[mqtt]     [%s] : %s\n", res.status.c_str(),
                    res.body.c_str());
    } while (!res.successful);

    Serial.println("xx");
    // verify the connection gets established
    do {
      delay(4000);
      mqs = this->_getMQTTConnectionStatus();
      Serial.println(mqs.toString().c_str());
      if ((int)mqs.state < MQTTState::ESTABLISHED) {
        Serial.printf(".%i", mqs.state);
      }
    } while ((int)mqs.state < MQTTState::ESTABLISHED);

    Serial.println(" ok");
    return true;
  }

  CIPStatus getCIPStatus();
  GMRInfo getGMRInfo();
  MQTTConnectionStatus& getMQTTConnectionStatus();
  IPAddress _getIPAddress(ATResult& res);
  std::string _getRawMacString(ATResult& res);

  void onIntervalMillis(uint32_t m, OnIntervalHandler h);
  bool connected() { return false; }

  ATResult execute(std::string, bool = true);
  ATResult query(std::string);

  void loop() { handleOnInterval(); }

  void handleOnInterval() {
    uint32_t now = millis();
    for (auto& i : _intervalHandlers) {
      if (now > _intervalTimers[i.first] + i.first) {
        _intervalTimers[i.first] = now;
        for (auto& func : i.second) {
          func();
        };
      }
    }
  }

  bool publish(const char* topic, const char* message) { return false; }
  bool publish(std::string topic, std::string message) { return false; }

  uint32_t getHeartbeatAge() { return _heartbeat; }

  bool subscribe(std::string topic) { return true; };
  bool subscribe(const char* topic) { return true; };

  std::string getIPAddress();

  bool disableEcho();
  bool enableEcho();
  bool isWifiConnected();
  bool isMQTTConnected();
  bool restoreFactoryDefault();

 private:
  uint32_t _heartbeat;
  uint16_t _atTTL = 20000;
  uint16_t _statusTTL = 60000;
  uint32_t _mqttConnStatusTime = 0;
  uint32_t _currLoop = 0;
  uint32_t _prevLoop = 0;

  bool _verbose = false;

  Stream* atSerial;

  std::string ssid;
  std::string psk;
  std::string server;
  std::string username;
  std::string password;
  std::string client;

  uint16_t port = 1883;

  GMRInfo _gmr;
  MQTTConnectionStatus _mqttConnStatus;

  std::string _response = "";
  std::string _end = "";
  std::string _body = "";

  IPAddress ip;
  std::string _ipStr;
  std::string _macStr;

  // Used by _readResponse;
  char _c;
  char _p;

  MQTTConnectionStatus _getMQTTConnectionStatus();
  void _readResponse();
  void _readReady();
  void __printATResult(ATResult& r);
  void __removeQuotes(TokenList& v);
};

// ==========================================================================
// Implmentations: AtMQTT
// ==========================================================================

ATResult AtMQTT::execute(std::string cmd, bool receipt) {
  ATResult res;
  uint32_t start = millis();

  if (cmd.empty()) {
    res.successful = false;
    res.status = "CLIENT ERROR";
    res.body =
        "You must provide a valid AT command. See:\n"
        "https://docs.espressif.com/projects/esp-at/en/latest/"
        "AT_Command_Set/index.html";
    return res;
  }

  _body = "";
  _response = "";

  atSerial->println(cmd.c_str());

  if (receipt == false) {
    res.successful = true;
    return res;
  }

  while (millis() < start + _atTTL) {
    if (atSerial->available()) {
      _readResponse();
    }
    if (_response == "OK" || _response == "ERROR") {
      break;
    }
  }

  // remove redundant newlines (if they exist)
  // may be multiple present
  while (!_body.empty() && _body[0] == '\n') {
    _body.erase(0, 1);
  }

  while (!_body.empty() && _body[_body.length() - 1] == '\n') {
    _body.erase(_body.length() - 1);
  }

  res.successful = _response == "OK" ? true : false;
  res.status = _response;
  res.body = _body;

  return res;
}

ATResult AtMQTT::query(std::string cmd) {
  ATResult res;
  if (cmd.empty()) {
    res.successful = false;
    res.status = "CLIENT ERROR";
    res.body =
        "You must provide a valid AT command. See:\n"
        "https://docs.espressif.com/projects/esp-at/en/latest/"
        "AT_Command_Set/index.html";
    return res;
  }

  if (cmd[cmd.length() - 1] != '?') {
    res.successful = false;
    res.status = "CLIENT ERROR";
    res.body = "Query commands must end i '?'";
    return res;
  }

  return this->execute(cmd);
}
/**
 * @brief At another interval event handler.
 *
 * @param m - interval (ms)
 * @param h - function handler
 */
void AtMQTT::onIntervalMillis(uint32_t m, OnIntervalHandler h) {
  auto it = _intervalHandlers.find(m);
  if (it == _intervalHandlers.end()) {
    _intervalHandlers[m] = OnIntervalHandlerList{h};
    _intervalTimers[m] = 0;
  } else {
    it->second.push_back(h);
  }
}

/**
 * @brief Get the general version information AT+GMR
 *
 * @return std::string
 */
GMRInfo AtMQTT::getGMRInfo() {
  _response = "";
  _body = "";

  if (_gmr.atVersion.length() > 0) {
    return _gmr;
  }

  ATResult res = this->execute("AT+GMR");

  if (!res.successful) {
    return _gmr;
  }

  std::string at = "AT version:";
  std::string sdk = "SDK version:";
  std::string time = "compile time";
  std::string bin = "Bin version:";

  std::size_t atPos = res.body.find(at);
  std::size_t sdkPos = res.body.find(sdk);
  std::size_t timePos = res.body.find(time);
  std::size_t binPos = res.body.find(bin);

  GMRInfo gmr;
  gmr.atVersion =
      res.body.substr(atPos + at.length(), sdkPos - at.length() - atPos - 1);
  gmr.sdkVersion = res.body.substr(sdkPos + sdk.length(),
                                   timePos - sdk.length() - sdkPos - 1);
  gmr.binVersion = res.body.substr(
      binPos + bin.length(), res.body.length() - binPos - bin.length() - 1);

  std::string timeStr = res.body.substr(timePos, binPos - timePos - 1);
  timePos = timeStr.find(":");
  gmr.compileTime = timeStr.substr(timePos + 1);

  _gmr = gmr;

  return _gmr;
}

bool AtMQTT::disableEcho() {
  _response = "";
  _body = "";

  ATResult res = this->execute("ATE0");
  return res.successful;
}

bool AtMQTT::enableEcho() {
  _response = "";
  _body = "";

  ATResult res = this->execute("ATE1");
  return res.successful;
}

/**
 * @brief Returns the current IP Address as string.
 *
 * @return std::string
 */
std::string AtMQTT::getIPAddress() {
  if (this->_ipStr.length() > 0) {
    return this->_ipStr;
  }

  ATResult res = this->execute("AT+CIFSR");

  if (!res.successful) {
    return "";
  }

  _getIPAddress(res);
  std::string mac = _getRawMacString(res);

  return _ipStr;
}

std::string AtMQTT::itos(int number) {
  char sport[64];
  sprintf(sport, "%i", number);

  return std::string{sport};
}

AtMQTT::TokenList AtMQTT::ssplit(std::string text, std::string separator) {
  std::size_t curr = 0, prev = 0;
  TokenList tokens;

  do {
    curr = text.find(separator, prev);

    if (curr == std::string::npos) {
      curr = text.length();
    }

    std::string token = text.substr(prev, curr - prev);
    prev = curr + separator.length();

    tokens.push_back(token);

  } while (curr < text.length() && prev < text.length());

  return tokens;
}

void AtMQTT::_readReady() {
  _c = atSerial->read();

  if (_c == '\r') {
    // skip the returns. they're noise
    return;
  }

  // Serial.printf("[%i] %c\n", _c, (_c == '\n') ? ' ' : _c);

  if (_c == '\n') {
    // return if response is OK or ERROR
    if (_response == "ready") {
      _p = _c;
      return;
    }

    // do not add multiple newlines.
    if (_c != _p) {
      _response += _c;
    }

    _body += _response;
    _response = "";
  } else {
    _p = _c;
    _response += _c;
  }

  // Serial.println(_response.c_str());
}

void AtMQTT::_readResponse() {
  _c = atSerial->read();

  if (_c == '\r') {
    // skip the returns. they're noise
    return;
  }

  Serial.printf("[%i] %c\n", _c, (_c == '\n') ? ' ' : _c);

  if (_c == '\n') {
    // return if response is OK or ERROR
    if (_response == "OK" || _response == "ERROR") {
      _p = _c;
      return;
    }

    // do not add multiple newlines.
    if (_c != _p) {
      _response += _c;
    }

    _body += _response;
    _response = "";
  } else {
    _p = _c;
    _response += _c;
  }

  // Serial.println(_response.c_str());
}

MQTTConnectionStatus AtMQTT::_getMQTTConnectionStatus() {
  ATResult res;
  MQTTConnectionStatus status;

  Serial.println("DEBUG");
  res = this->query("AT+MQTTCONN?");
  Serial.println(res.body.c_str());

  std::string info = res.body.substr(res.body.find("MQTTCONN:") + 9);
  TokenList data = ssplit(info, ",");

  __removeQuotes(data);

  bool reconnect = data[6] == "1" ? true : false;

  status = {(uint8_t)atoi(data[0].c_str()),
            (MQTTState)atoi(data[1].c_str()),
            (MQTTScheme)atoi(data[2].c_str()),
            data[3],
            (uint16_t)atoi(data[4].c_str()),
            data[5],
            reconnect};

  return status;
}
MQTTConnectionStatus& AtMQTT::getMQTTConnectionStatus() {
  uint32_t now = millis();
  if (_mqttConnStatusTime > 0 && now < _mqttConnStatusTime + _statusTTL) {
    return _mqttConnStatus;
  }

  _mqttConnStatus = _getMQTTConnectionStatus();
  _mqttConnStatusTime = now;
  return _mqttConnStatus;
}

std::string AtMQTT::_getRawMacString(ATResult& res) {
  if (_macStr.length() > 0) {
    return _macStr;
  }

  std::string mac;
  std::size_t startP;
  std::size_t endP;

  startP = res.body.find("STAMAC,\"");
  mac = res.body.substr(startP + 8);
  endP = mac.find("\"");
  _macStr = mac.substr(0, endP);
  return _macStr;
}

IPAddress AtMQTT::_getIPAddress(ATResult& res) {
  std::string ips;
  std::size_t startP;
  std::size_t endP;

  startP = res.body.find("STAIP,\"");
  ips = res.body.substr(startP + 7);
  endP = ips.find("\"");
  ips = ips.substr(0, endP);

  this->_ipStr = ips;

  uint8_t octets[4];
  uint8_t i = 0;
  std::size_t curr = 0, prev = 0;

  do {
    curr = ips.find(".", prev);

    if (curr == std::string::npos) {
      curr = ips.length();
    }

    std::string token = ips.substr(prev, curr - prev);
    prev = curr + 1;

    octets[i] = atoi(token.c_str());
    i++;

  } while (curr < ips.length() && prev < ips.length());

  IPAddress ip(octets[0], octets[1], octets[2], octets[3]);
  return ip;
}

/**
 * @brief resets the at device to factory defaults
 *
 * @return true
 * @return false
 */
bool AtMQTT::restoreFactoryDefault() {
  ATResult res = this->execute("AT+RESTORE");

  if (!res.successful) {
    return false;
  }

  _body = "";
  _response = "";

  uint32_t start = millis();
  while (millis() < start + _atTTL) {
    if (atSerial->available()) {
      _readReady();
      // Serial.printf("[%s]  [%s]\n", _response.c_str(), _body.c_str());
    }
    if (_response == "ready") {
      break;
    }
  }

  return true;
}

bool AtMQTT::isMQTTConnected() {
  ATResult res;

  MQTTConnectionStatus status = _getMQTTConnectionStatus();
  _mqttConnStatus = status;
  _mqttConnStatusTime = millis();

  Serial.println(status.toString().c_str());

  if ((int)status.state >= (int)MQTTState::ESTABLISHED) {
    return true;
  }

  return false;
}

bool AtMQTT::isWifiConnected() {
  ATResult res;
  res.successful = false;

  res = this->query("AT+CWJAP?");
  __printATResult(res);

  if (!res.successful) {
    return false;
  }

  std::string info = res.body.substr(res.body.find("CWJAP:") + 6);
  std::vector<std::string> data = ssplit(info, ",");

  if (data.size() == 0) {
    return false;
  }

  // Serial.println(data[0].c_str());
  if (!data[0].empty() && data[0][0] == '"') {
    data[0].erase(0, 1);
  }

  if (!data[0].empty() && data[0][data[0].length() - 1] == '"') {
    data[0].erase(data[0].length() - 1);
  }

  if (data[0] == this->ssid) {
    return true;
  }

  return false;
}

void AtMQTT::__removeQuotes(AtMQTT::TokenList& v) {
  for (auto& i : v) {
    if (!i.empty() && i[0] == '"') {
      i.erase(0, 1);
    }

    if (!i.empty() && i[i.length() - 1] == '"') {
      i.erase(i.length() - 1);
    }
  }
}

void AtMQTT::__printATResult(ATResult& r) {
  Serial.printf("{\n");
  Serial.printf("  successful: %s\n", r.successful ? "true" : "false");
  Serial.printf("  status: '%s'\n", r.status.c_str());
  Serial.printf("  body: '%s'\n", r.body.c_str());
  Serial.printf("}\n");
}

CIPStatus AtMQTT::getCIPStatus() {
  ATResult res = this->execute("AT+CIPSTATUS");
  if (!res.successful) {
    return CIPStatus::IDLE;
  }

  return (CIPStatus)atoi(
      res.body.substr(res.body.find("STATUS:") + 7, 1).c_str());
}

// ==========================================================================
// Implmentations: Other classes
// ==========================================================================

std::string MQTTConnectionStatus::toString() {
  std::string nl = "'\n";
  std::string s = "{\n";
  s += "  linkId: '" + AtMQTT::itos(linkId) + nl;
  s += "  state: '" + AtMQTT::itos(state) + nl;
  s += "  scheme: '" + AtMQTT::itos(scheme) + nl;
  s += "  hostname: '" + hostname + nl;
  s += "  port: '" + AtMQTT::itos(port) + nl;
  s += "  path: '" + path + nl;
  s += "  reconnect: '" + std::string{(reconnect ? "true" : "false")} + nl +
       "}\n";
  return s;
}

#endif  // ATMQTT_H
