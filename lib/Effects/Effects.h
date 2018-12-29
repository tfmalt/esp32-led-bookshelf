#ifndef Effects_h
#define Effects_h

#include <FastLED.h>
#include <LightState.h>
#include <Arduino.h>
#include <ArduinoOTA.h>

class Effects {
    public:
        enum Command {
            Null,
            None,
            Empty,
            Brightness,
            GlobalBrightness,
            Color,
            FirmwareUpdate,
            TurnOff,
            TurnOn
        };
        enum Effect {
            Confetti,
            BPM,
            GlitterRainbow,
            Rainbow,
            RainbowByShelf,
            Juggle,
            Sinelon,
            NullEffect,
            EmptyEffect,
            NoEffect
        };
        Effects();
        Effects(LightState* l, uint8_t f);

        Command  currentCommandType;
        Effect   currentEffectType;
        ulong    commandStart = 0;

        void setCurrentCommand(Command cmd);
        void setCurrentEffect(String effect);
        void setCurrentEffect(Effect effect);
        Effect getEffectFromString(String str);
        void runCurrentCommand();
        void runCurrentEffect();
        void setFPS(uint8_t f);
        void setLightState(LightState *l);
        void setCommandFrames(uint16_t i);
        void setLeds(CRGB* l);
        void setLeds(CRGB* l, const uint16_t &n);
        Effect getCurrentEffect();
        void setStartHue(float hue);
        void setTop(uint16_t start, uint16_t stop);
        void setBottom(uint16_t start, uint16_t stop);

    private:
        typedef void (Effects::*LightCmd)();
        LightCmd     currentCommand;
        LightCmd     currentEffect;
        LightState*  lightState;
        CRGB*        leds;

        uint8_t  FPS                 = 0;
        uint16_t numberOfLeds        = 0;
        uint8_t  startHue            = 0;
        uint16_t commandFrameCount   = 0;
        uint16_t commandFrames       = 0;

        uint16_t topStart    = 0;
        uint16_t topStop     = 0;
        uint16_t bottomStart = 0;
        uint16_t bottomStop  = 0;

        void initialize();
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
        void effectRainbowByShelf();
        void effectBPM();
        void effectConfetti();
        void effectSinelon();
        void effectJuggle();
};

#endif // Effects_h