# LED Bookshelf Controller
A programmable LED controller with full [Home Assistant](https://www.home-assistant.io/) integration.

## Goals
- Brush the dust of my 20 year old C/C++ knowledge
- Familiarise myself with the ESP32
- Do proof of concept for a modern cloud centric IoT architecture
- Create a really cool bookshelf

## Functionality 
- Communicates with MQTT over TLS
- Full MQTT controll interface
- Implemenents the [MQTT Light json](https://www.home-assistant.io/integrations/light.mqtt/) interface for talking to a home assistant server
- Controls WS2812B or SK9822 ledstrips to create a beautiful bookshelf
- Uses the SPIFFS file system library to separate configuration from controller logic.
- Supports OTA firmware updates
- Audio input with FFT frequency analysis to create audio responsive light displays.

## TODO
- Implement custom UI and options beyond what the deafult home assistant interface offers.

## Demonstration
[![Demonstration video of working led lights](https://img.youtube.com/vi/cJR5gxJv22c/0.jpg)](https://www.youtube.com/watch?v=cJR5gxJv22c)

### References
- http://fastled.io/
- https://www.espressif.com/en/products/hardware/esp32/overview
- https://www.home-assistant.io/
- https://www.home-assistant.io/components/light.mqtt_json/
