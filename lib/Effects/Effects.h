#ifndef Effects_h
#define Effects_h

#include <FastLED.h>
#include <LightStateController.h>
#include <Arduino.h>

class Effects {
    private:
        typedef void (Effects::*LightCmd)();
        LightCmd currentCommand;
        LightCmd currentEffect;
        LightStateController *lightState;

        uint8_t     FPS                 = 0;
        uint16_t    commandFrameCount   = 0;
        uint16_t    commandFrames       = 0;
        ulong       commandStart        = 0;

        void cmdEmpty();
        void cmdSetBrightness();
    public:
        enum Command {
            Null,
            None,
            Empty,
            Brightness
        };
        enum Effect {

        };
        Effects();
        void setCurrentCommand(Command cmd);
        void runCurrentCommand();
        void setFPS(uint8_t f);
        void setLightStateController(LightStateController *l);
        void setCommandFrames(uint16_t i);
};

#endif // Effects_h