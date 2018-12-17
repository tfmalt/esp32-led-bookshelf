#ifndef Effects_h
#define Effects_h

#include <FastLED.h>
#include <LightStateController.h>
#include <MQTTController.h>
#include <Arduino.h>
#include <ArduinoOTA.h>

class MQTTController;

class Effects {
    private:
        typedef void            (Effects::*LightCmd)();
        LightCmd                currentCommand;
        LightCmd                currentEffect;
        LightStateController    *lightState;
        MQTTController          *mqttCtrl;
        CRGB                    *leds;

        uint8_t                 FPS                 = 0;
        uint16_t                numberOfLeds        = 0;
        uint8_t                 startHue            = 0;
        uint16_t                commandFrameCount   = 0;
        uint16_t                commandFrames       = 0;
        ulong                   commandStart        = 0;

        void cmdEmpty();
        void cmdSetBrightness();
        void cmdFadeTowardColor();
        void cmdFirmwareUpdate();

        void fadeTowardColor(CRGB *L, uint16_t N, const CRGB &bgColor, uint8_t fadeAmount);
        CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount);
        void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount);
        void addGlitter(fract8 chanceOfGlitter);
        void effectGlitterRainbow();
        void effectRainbow();
        void effectBPM();
        void effectConfetti();
        void effectSinelon();
        void effectJuggle();

    public:
        enum Command {
            Null,
            None,
            Empty,
            Brightness,
            Color,
            FirmwareUpdate
        };
        enum Effect {
            Confetti,
            BPM,
            GlitterRainbow,
            Rainbow,
            Juggle,
            Sinelon,
            NullEffect,
            EmptyEffect,
            NoEffect
        };
        Command  currentCommandType;
        Effect   currentEffectType;
        Effects();
        void setCurrentCommand(Command cmd);
        void setCurrentEffect(String effect);
        void setCurrentEffect(Effect effect);
        Effect getEffectFromString(String str);
        void runCurrentCommand();
        void runCurrentEffect();
        void setFPS(uint8_t f);
        void setLightStateController(LightStateController *l);
        void setMQTTController(MQTTController *m);
        void setCommandFrames(uint16_t i);
        void setLeds(CRGB *l, const uint16_t &n);
        Effect getCurrentEffect();
        void setStartHue(float hue);
};

#endif // Effects_h