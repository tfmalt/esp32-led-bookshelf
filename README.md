# LED Bookshelf Controller
My first attempt at writing a led controller for esp32. 

## Goals
- Brush the dust of my 20 year old C/C++ knowledge
- Familiarise myself with the ESP32
- Do proof of concept for a modern cloud centric IoT architecture
- Create a really cool bookshelf

## Functionality 
- Communicates with MQTT over TLS
- Implemenents the mqtt_json protocol for talking to a home assistant server
- Controls WS2812B ledstrips to create a beautiful bookshelf
- Uses the SPIFFS file system library to separate configuration from controller logic.

## Todo 
- Implement OTA firmware updates
- Implement web UI to change configuration settings

## Demonstration
[![Demonstration video of working led lights](https://img.youtube.com/vi/cJR5gxJv22c/0.jpg)](https://www.youtube.com/watch?v=cJR5gxJv22c)

### References
- http://fastled.io/
- https://www.espressif.com/en/products/hardware/esp32/overview
- https://www.home-assistant.io/
- https://www.home-assistant.io/components/light.mqtt_json/
