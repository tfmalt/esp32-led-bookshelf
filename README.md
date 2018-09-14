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

### References
- https://www.home-assistant.io/components/light.mqtt_json/
