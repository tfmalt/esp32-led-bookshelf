#ifndef Effects_h
#define Effects_h

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <FastLED.h>
#include <LightStateController.h>

#ifdef FFT_ACTIVE
#include <arduinoFFT.h>
#endif

class Effects {
 private:
  typedef void (Effects::*LightCmd)();
  LightCmd currentCommand;
  LightCmd currentEffect;
  LightStateController *lightState;
  CRGB *leds;

  uint16_t numberOfLeds = LED_COUNT;
  uint8_t startHue = 0;
  uint8_t confettiHue = 0;
  uint16_t commandFrameCount = 0;
  uint16_t commandFrames = 0;

  unsigned long sampleDelay();
  void fftComputeSampleset();
  void fftFillBuckets();
  void fftFillBucketsSimple();
  void cmdEmpty();
  void cmdSetBrightness();
  void cmdFadeTowardColor();
  void cmdFirmwareUpdate();

  void fadeTowardColor(CRGB *L, uint16_t N, const CRGB &bgColor,
                       uint8_t fadeAmount);
  CRGB fadeTowardColor(CRGB &cur, const CRGB &target, uint8_t amount);
  void nblendU8TowardU8(uint8_t &cur, const uint8_t target, uint8_t amount);
  void addGlitter(fract8 chanceOfGlitter);

  void effectGlitterRainbow();
  void effectRainbow();
  void effectRainbowByShelf();
  void effectBPM();
  void effectVUMeter();
  void effectMusicDancer();
  void effectConfetti();
  void effectSinelon();
  void effectJuggle();
  void effectFrequencies();

 public:
  enum Command { Null, None, Empty, Brightness, Color, FirmwareUpdate };
  enum Effect {
    Confetti,
    BPM,
    GlitterRainbow,
    Rainbow,
    RainbowByShelf,
    Juggle,
    Sinelon,
    VUMeter,
    MusicDancer,
    Frequencies,
    MSGSerial,
    NullEffect,
    EmptyEffect,
    NoEffect
  };

  Command currentCommandType;
  Effect currentEffectType;
  ulong commandStart = 0;
  Effects();
  void setCurrentCommand(Command cmd);
  void setCurrentEffect(String effect);
  void setCurrentEffect(Effect effect);
  Effect getEffectFromString(String str);
  void runCurrentCommand();
  void runCurrentEffect();
  // void setFPS(uint8_t f);
  void setLightStateController(LightStateController *l);
  void setCommandFrames(uint16_t i);
  void setLeds(CRGB *l, const uint16_t &n);
  Effect getCurrentEffect();
  void setStartHue(float hue);
};

#endif  // Effects_h