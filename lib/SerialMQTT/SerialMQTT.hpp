#ifndef SERIALMQTT_H
#define SERIALMQTT_H
#ifdef IS_TEENSY

#include <Arduino.h>

#define BAUD_RATE 962100

class SerialMQTT {
 public:
  SerialMQTT() {}

  void setup() {
    // Setting up serial connection
    Serial3.begin(BAUD_RATE, SERIAL_8N1);
  }

  void handle() {
    if (Serial3.available() > 0) {
      readData();
    }
  }

 private:
  void readData() {
    char c;
    char end = '\n';

    while (Serial3.available() > 0) {
      c = Serial3.read();
    }
  }
};

#endif  // IS_TEENSY
#endif  // SERIALMQTT_H