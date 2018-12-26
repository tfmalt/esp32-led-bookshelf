
#include "Effects.h"
// #include <FastLED.h>

Effects::Effects() {
    currentCommand = &Effects::cmdEmpty;
    currentEffect  = &Effects::cmdEmpty;

    currentCommandType = Command::None;
    currentEffectType  = Effect::NullEffect;
}

void Effects::setFPS(uint8_t f)
{
    FPS = f;
}

void Effects::setLightStateController(LightStateController *l)
{
    lightState = l;
}

void Effects::setCommandFrames(uint16_t i)
{
    commandFrames = i;
}

void Effects::setCurrentCommand(Command cmd)
{
    LightState state   = lightState->getCurrentState();
    commandFrameCount  = 0;
    commandStart       = millis();
    currentCommandType = cmd;

    setCommandFrames(FPS * (state.transition || 1));

    switch(cmd) {
        case Command::Brightness :
            currentCommand = &Effects::cmdSetBrightness;
            break;
        case Command::Color :
            currentCommand = &Effects::cmdFadeTowardColor;
            break;
        case Command::FirmwareUpdate :
            currentCommand = &Effects::cmdFirmwareUpdate;
            break;
        default :
            currentCommand = &Effects::cmdEmpty;
            currentCommandType = Command::None;
    }
}

void Effects::setCurrentEffect(String effect)
{
    setCurrentEffect(getEffectFromString(effect));
}

Effects::Effect Effects::getEffectFromString(String str)
{
    if (str == "Glitter Rainbow")   return Effect::GlitterRainbow;
    if (str == "Rainbow")           return Effect::Rainbow;
    if (str == "RainbowByShelf")    return Effect::RainbowByShelf;
    if (str == "BPM")               return Effect::BPM;
    if (str == "Confetti")          return Effect::Confetti;
    if (str == "Juggle")            return Effect::Juggle;
    if (str == "Sinelon")           return Effect::Sinelon;

    return Effect::NullEffect;
}

void Effects::setCurrentEffect(Effect effect)
{
    LightState state = lightState->getCurrentState();
    currentEffectType = effect;

    switch(effect) {
        case Effect::GlitterRainbow :
            currentEffect = &Effects::effectGlitterRainbow;
            break;
        case Effect::Rainbow :
            currentEffect = &Effects::effectRainbow;
            break;
        case Effect::RainbowByShelf :
            currentEffect = &Effects::effectRainbowByShelf;
            break;
        case Effect::BPM :
            currentEffect = &Effects::effectBPM;
            break;
        case Effect::Confetti :
            currentEffect = &Effects::effectConfetti;
            break;
        case Effect::Sinelon :
            currentEffect = &Effects::effectSinelon;
            break;
        case Effect::Juggle :
            currentEffect = &Effects::effectJuggle;
            break;
        default :
            currentEffectType = Effect::NullEffect;
            currentEffect     = &Effects::cmdEmpty;
    }
}

void Effects::runCurrentCommand()
{
    (this->*currentCommand)();
}

void Effects::runCurrentEffect()
{
    (this->*currentEffect)();
}

void Effects::cmdEmpty()
{

}

void Effects::cmdFirmwareUpdate()
{
    fill_solid(leds, 15, CRGB::Black);
    // fill_solid(leds, 15, CRGB::White);
    leds[3] = CRGB::White;
    leds[7] = CRGB::White;
    leds[11] = CRGB::White;
    leds[15] = CRGB::White;

    FastLED.show();
}

void Effects::cmdSetBrightness()
{
    LightState state = lightState->getCurrentState();

    uint8_t  target   = state.brightness;
    uint8_t  current  = FastLED.getBrightness();

    if (commandFrameCount < commandFrames) {
        FastLED.setBrightness(current + ((target - current) / (commandFrames - commandFrameCount)));
        commandFrameCount++;
    } else {
        Serial.printf(
            "  - command: setting brightness DONE [%i] %lu ms.\n",
            FastLED.getBrightness(), (millis() - commandStart)
        );

        setCurrentCommand(Command::None);
    }
}

/**
 * Helper function that blends one uint8_t toward another by a given amount
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void Effects::nblendU8TowardU8( uint8_t& cur, const uint8_t target, uint8_t amount)
{
    if( cur == target) return;

    if( cur < target ) {
        uint8_t delta = target - cur;
        delta = scale8_video( delta, amount);
        cur += delta;
    } else {
        uint8_t delta = cur - target;
        delta = scale8_video( delta, amount);
        cur -= delta;
    }
}

/**
 * Blend one CRGB color toward another CRGB color by a given amount.
 * Blending is linear, and done in the RGB color space.
 * This function modifies 'cur' in place.
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
CRGB Effects::fadeTowardColor( CRGB& cur, const CRGB& target, uint8_t amount)
{
    nblendU8TowardU8( cur.red,   target.red,   amount);
    nblendU8TowardU8( cur.green, target.green, amount);
    nblendU8TowardU8( cur.blue,  target.blue,  amount);
    return cur;
}

/**
 * Fade an entire array of CRGBs toward a given background color by a given amount
 * This function modifies the pixel array in place.
 * Taken from: https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void Effects::fadeTowardColor( CRGB* L, uint16_t N, const CRGB& bgColor, uint8_t fadeAmount)
{
    uint16_t check = 0;
    for( uint16_t i = 0; i < N; i++) {
        fadeTowardColor( L[i], bgColor, fadeAmount);
        if (L[i] == bgColor) check++;
    }

    if (check == numberOfLeds) {
        // currentCommand = cmdEmpty;
        Serial.printf("  - fade towards color done in %lu ms.\n", (millis() - commandStart));
        setCurrentCommand(Command::None);
    }
}

void Effects::cmdFadeTowardColor()
{
    LightState state = lightState->getCurrentState();
    CRGB targetColor(state.color.r, state.color.g, state.color.b);

    fadeTowardColor(leds, numberOfLeds, targetColor, 12);
}

void Effects::setLeds(CRGB* l, const uint16_t &n)
{
    numberOfLeds = n;
    leds = l;
}

void Effects::effectRainbow()
{
    fill_rainbow(leds, numberOfLeds, startHue, 2);
}

void Effects::effectRainbowByShelf()
{
    CRGBSet ledset(leds, numberOfLeds);
    ledset(0, 63).fill_rainbow(startHue, 4);
    ledset(64, 127) = ledset(63, 0);

    if (numberOfLeds < 131) return;

    ledset(128, 191).fill_rainbow(startHue+64, 4);
    ledset(192, 255) = ledset(191,128);

    if (numberOfLeds < 257) return;

    ledset(256, 319).fill_rainbow(startHue+128, 4);
    ledset(320, 383) = ledset(319, 256);
}

void Effects::addGlitter(fract8 chanceOfGlitter)
{
    if( random8() < chanceOfGlitter) {
        leds[ random16(numberOfLeds) ] += CRGB::White;
    }
}

void Effects::effectGlitterRainbow()
{
    // built-in FastLED rainbow, plus some random sparkly glitter
    effectRainbow();
    addGlitter(80);
}

void Effects::effectConfetti()
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, numberOfLeds, 20);
  int pos = random16(numberOfLeds);
  leds[pos] += CHSV( startHue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void Effects::effectSinelon()
{
  fadeToBlackBy( leds, numberOfLeds, 60 );
  LightState state = lightState->getCurrentState();

    // calculate a suiting pulserate for the number of leds.
  uint8_t bpm = 1000 / numberOfLeds;

  int pos = beatsin16(bpm, 0, numberOfLeds-1 );
  // leds[pos] += CHSV( startHue, 255, 255);
  leds[pos] += CRGB(state.color.r, state.color.g, state.color.b);
}

/**
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
void Effects::effectBPM()
{
    uint8_t BeatsPerMinute = 64;
    CRGBPalette16 palette = PartyColors_p;
    uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
    for( int i = 0; i < numberOfLeds; i++) { //9948
        leds[i] = ColorFromPalette(palette, startHue+(i*2), beat-startHue+(i*10));
    }
}

/**
 * eight colored dots, weaving in and out of sync with each other
 */
void Effects::effectJuggle()
{
  fadeToBlackBy( leds, numberOfLeds, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, numberOfLeds-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

Effects::Effect Effects::getCurrentEffect()
{
    return currentEffectType;
}

void Effects::setStartHue(float hue)
{
    startHue = static_cast<uint8_t>(hue*(256.0/360.0));
}
