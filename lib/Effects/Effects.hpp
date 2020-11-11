#ifndef Effects_h
#define Effects_h

#include <Arduino.h>
#include <FastLED.h>

#include <LightState.hpp>
#include <functional>
#include <map>

namespace Effects {

typedef enum { Null, None, Empty, Brightness, Color, FirmwareUpdate } Command;
typedef enum {
  Confetti,
  BPM,
  GlitterRainbow,
  Rainbow,
  Gradient,
  RainbowByShelf,
  Juggle,
  Sinelon,
  Pride,
  Colorloop,
  WalkingRainbow,
  VUMeter,
  MusicDancer,
  Frequencies,
  NullEffect,
  EmptyEffect,
  NoEffect
} Effect;

typedef std::function<void()> EffectFunction;
typedef std::function<void()> CmdFunction;
typedef std::map<Command, CmdFunction> CmdQueue;

class Controller {
 private:
  // typedef void (Effects::Controller::*LightCmd)();
  // LightCmd currentCommand;
  CmdQueue cmdQueue;
  EffectFunction currentEffect;
  LightState::LightState state;
  CRGB *leds;

  uint16_t numberOfLeds = LED_COUNT;
  uint8_t startHue = 0;
  uint8_t confettiHue = 0;
  uint16_t commandFrameCount = 0;
  uint16_t commandFrames = 0;

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
  void effectPride();
  void effectColorloop();
  void effectGradient();
  void effectWalkingRainbow();
  void effectMusicDancer();
  void effectConfetti();
  void effectSinelon();
  void effectJuggle();
  void effectFrequencies();

  void setInitialState();

 public:
  Command currentCommandType;
  Effect currentEffectType;
  ulong commandStart = 0;

  Controller() {
    // this->currentCommand = &Effects::Controller::cmdEmpty;
    this->currentEffect = []() {};

    this->currentCommandType = Command::None;
    this->currentEffectType = Effect::NullEffect;
  };

  void setup(CRGB *l, const uint16_t n, LightState::LightState s);

  void handleStateChange(LightState::LightState state);
  void setCurrentCommand(Command cmd);
  void setCurrentEffect(std::string effect);
  void setCurrentEffect(Effect effect);
  Effect getEffectFromString(std::string str);
  void runCurrentCommand();
  void runCurrentEffect();
  // void setFPS(uint8_t f);
  // void setLightStateController(LightState::Controller *l);
  void setCommandFrames(uint16_t i);
  Effect getCurrentEffect();
  void setStartHue(float hue);
};
}  // namespace Effects

#endif  // Effects_h