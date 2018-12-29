#ifndef LightController_h
#define LightController_h

#include <Arduino.h>
#include <FastLED.h>
#include <Effects.h>
#include <LightState.h>
#include <LedshelfConfig.h>

class Light {
    public:
        enum Segment {
            Top,
            Bottom,
            Left,
            Right
        };

        Light();
        Light(CRGB* l);
        Light(CRGB* l, uint8_t fps, LightConfig lc, String username);

        Light&  addLeds(CRGB* l);
        Light&  addSegment(Segment segment, uint16_t start, uint16_t stop);
        Light&  setCommandTopic(const char* topic);
        Light&  setStateTopic(const char* topic);
        Light&  setFPS(uint8_t fps);
        uint8_t getBrightness();
        String  getStateAsJSON();
        String  getName();
        String  getStateTopic();
        String  getCommandTopic();
        void    onCommand(byte* message);
        bool    isMaster();
        bool    isMaster(bool m);

    private:
        Effects     effects;
        LightState  state;

        CRGB*       leds;
        uint8_t     FPS;
        bool        master = false;

        uint16_t    topStart        = 0;
        uint16_t    topStop         = 0;
        uint16_t    bottomStart     = 0;
        uint16_t    bottomStop      = 0;

        String command_topic;
        String state_topic;
        String name;
        String filename;

        void handleNewState();
        void handleColor(Color c);
        void handleBrightness(uint8_t brightness);
        void handleEffect(String effect);
        void handleColorTemp(uint16_t colorTemp);

        uint8_t getColorTempRed(uint16_t ct);
        uint8_t getColorTempGreen(uint16_t ct);
        uint8_t getColorTempBlue(uint16_t ct);

};

#endif // LightController_h