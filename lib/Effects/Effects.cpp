
#include "Effects.h"

// #define FFT_SAMPLES 1024
// #define FFT_BUCKETS 8
// #define EQ_SEGMENT 8

arduinoFFT FFT = arduinoFFT();

double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];
int buckets[FFT_BUCKETS] = {0};
int segment = LED_COUNT / FFT_BUCKETS;

Effects::Effects() {
  currentCommand = &Effects::cmdEmpty;
  currentEffect = &Effects::cmdEmpty;

  currentCommandType = Command::None;
  currentEffectType = Effect::NullEffect;
}

void Effects::setLightStateController(LightStateController* l) {
  lightState = l;
}

void Effects::setCommandFrames(uint16_t i) { commandFrames = i; }

void Effects::setCurrentCommand(Command cmd) {
  LightState state = lightState->getCurrentState();
  commandFrameCount = 0;
  commandStart = millis();
  currentCommandType = cmd;

  setCommandFrames(FPS * (state.transition || 1));

  switch (cmd) {
    case Command::Brightness:
      currentCommand = &Effects::cmdSetBrightness;
      break;
    case Command::Color:
      currentCommand = &Effects::cmdFadeTowardColor;
      break;
    case Command::FirmwareUpdate:
      currentCommand = &Effects::cmdFirmwareUpdate;
      break;
    default:
      currentCommand = &Effects::cmdEmpty;
      currentCommandType = Command::None;
  }
}

void Effects::setCurrentEffect(String effect) {
#ifdef DEBUG
  Serial.printf("  - Effects: Setting new effect: %s\n", effect);
#endif
  setCurrentEffect(getEffectFromString(effect));
}

Effects::Effect Effects::getEffectFromString(String str) {
  if (str == "Glitter Rainbow") return Effect::GlitterRainbow;
  if (str == "Rainbow") return Effect::Rainbow;
  if (str == "RainbowByShelf") return Effect::RainbowByShelf;
  if (str == "BPM") return Effect::BPM;
  if (str == "Confetti") return Effect::Confetti;
  if (str == "Juggle") return Effect::Juggle;
  if (str == "Sinelon") return Effect::Sinelon;
  if (str == "Equalizer") return Effect::Equalizer;

  return Effect::NullEffect;
}

void Effects::setCurrentEffect(Effect effect) {
  LightState state = lightState->getCurrentState();
  currentEffectType = effect;

  switch (effect) {
    case Effect::GlitterRainbow:
      currentEffect = &Effects::effectGlitterRainbow;
      break;
    case Effect::Rainbow:
      currentEffect = &Effects::effectRainbow;
      break;
    case Effect::RainbowByShelf:
      currentEffect = &Effects::effectRainbowByShelf;
      break;
    case Effect::BPM:
      currentEffect = &Effects::effectBPM;
      break;
    case Effect::Equalizer:
      currentEffect = &Effects::effectEqualizer;
      break;
    case Effect::Confetti:
      currentEffect = &Effects::effectConfetti;
      break;
    case Effect::Sinelon:
      currentEffect = &Effects::effectSinelon;
      break;
    case Effect::Juggle:
      currentEffect = &Effects::effectJuggle;
      break;
    default:
      currentEffectType = Effect::NullEffect;
      currentEffect = &Effects::cmdEmpty;
  }
}

void Effects::runCurrentCommand() { (this->*currentCommand)(); }

void Effects::runCurrentEffect() { (this->*currentEffect)(); }

void Effects::cmdEmpty() {}

void Effects::cmdFirmwareUpdate() {
  fill_solid(leds, 15, CRGB::Black);
  // fill_solid(leds, 15, CRGB::White);
  leds[3] = CRGB::White;
  leds[7] = CRGB::White;
  leds[11] = CRGB::White;
  leds[15] = CRGB::White;

  FastLED.show();
}

void Effects::cmdSetBrightness() {
  LightState state = lightState->getCurrentState();

  uint8_t target = state.brightness;
  uint8_t current = FastLED.getBrightness();

  if (commandFrameCount < commandFrames) {
    FastLED.setBrightness(
        current + ((target - current) / (commandFrames - commandFrameCount)));
    commandFrameCount++;
  } else {
#ifdef DEBUG
    Serial.printf("  - command: setting brightness DONE [%i] %lu ms.\n",
                  FastLED.getBrightness(), (millis() - commandStart));
#endif

    setCurrentCommand(Command::None);
  }
}

/**
 * Helper function that blends one uint8_t toward another by a given amount
 * Taken from:
 * https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void Effects::nblendU8TowardU8(uint8_t& cur, const uint8_t target,
                               uint8_t amount) {
  if (cur == target) return;

  if (cur < target) {
    uint8_t delta = target - cur;
    delta = scale8_video(delta, amount);
    cur += delta;
  } else {
    uint8_t delta = cur - target;
    delta = scale8_video(delta, amount);
    cur -= delta;
  }
}

/**
 * Blend one CRGB color toward another CRGB color by a given amount.
 * Blending is linear, and done in the RGB color space.
 * This function modifies 'cur' in place.
 * Taken from:
 * https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
CRGB Effects::fadeTowardColor(CRGB& cur, const CRGB& target, uint8_t amount) {
  nblendU8TowardU8(cur.red, target.red, amount);
  nblendU8TowardU8(cur.green, target.green, amount);
  nblendU8TowardU8(cur.blue, target.blue, amount);
  return cur;
}

/**
 * Fade an entire array of CRGBs toward a given background color by a given
 * amount This function modifies the pixel array in place. Taken from:
 * https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void Effects::fadeTowardColor(CRGB* L, uint16_t N, const CRGB& bgColor,
                              uint8_t fadeAmount) {
  uint16_t check = 0;
  for (uint16_t i = 0; i < N; i++) {
    fadeTowardColor(L[i], bgColor, fadeAmount);
    if (L[i] == bgColor) check++;
  }

  if (check == numberOfLeds) {
#ifdef DEBUG
    Serial.printf("  - fade towards color done in %lu ms.\n",
                  (millis() - commandStart));
#endif
    setCurrentCommand(Command::None);
  }
}

void Effects::cmdFadeTowardColor() {
  LightState state = lightState->getCurrentState();
  CRGB targetColor(state.color.r, state.color.g, state.color.b);

  fadeTowardColor(leds, numberOfLeds, targetColor, 12);
}

void Effects::setLeds(CRGB* l, const uint16_t& n) {
  numberOfLeds = n;
  leds = l;
}

void Effects::effectRainbow() { fill_rainbow(leds, numberOfLeds, startHue, 2); }

void Effects::effectRainbowByShelf() {
  CRGBSet ledset(leds, numberOfLeds);
  ledset(0, 63).fill_rainbow(startHue, 4);
  ledset(64, 127) = ledset(63, 0);

  if (numberOfLeds < 131) return;

  ledset(128, 191).fill_rainbow(startHue + 64, 4);
  ledset(192, 255) = ledset(191, 128);

  if (numberOfLeds < 257) return;

  ledset(256, 319).fill_rainbow(startHue + 128, 4);
  ledset(320, 383) = ledset(319, 256);
}

void Effects::addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(numberOfLeds)] += CRGB::White;
  }
}

void Effects::effectGlitterRainbow() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  effectRainbow();
  addGlitter(80);
}

void Effects::effectConfetti() {
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy(leds, numberOfLeds, 20);
  int pos = random16(numberOfLeds);
  leds[pos] += CHSV(startHue + random8(64), 200, 255);
}

// a colored dot sweeping back and forth, with fading trails
void Effects::effectSinelon() {
  fadeToBlackBy(leds, numberOfLeds, 60);
  LightState state = lightState->getCurrentState();

  // calculate a suiting pulserate for the number of leds.
  uint8_t bpm = 1000 / numberOfLeds;

  int pos = beatsin16(bpm, 0, numberOfLeds - 1);
  // leds[pos] += CHSV( startHue, 255, 255);
  leds[pos] += CRGB(state.color.r, state.color.g, state.color.b);
}

/**
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
void Effects::effectBPM() {
  uint8_t BeatsPerMinute = 64;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < numberOfLeds; i++) {  // 9948
    leds[i] = ColorFromPalette(palette, startHue + (i * 2),
                               beat - startHue + (i * 10));
  }
}

#define SAMPLING_FREQUENCY 40000
unsigned long newtime;
unsigned long oldtime;
unsigned long sampling_period = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

/**
 * FFT based spectrum analyzer disco lights
 */
void Effects::effectEqualizer() {
  CRGBSet ledset(leds, numberOfLeds);

  ledset(0, FFT_BUCKETS * segment) = CRGB::Black;

  // Serial.println("Running effect equalizer");
  for (int i = 0; i < FFT_SAMPLES; i++) {
    newtime = micros() - oldtime;
    oldtime = newtime;

    vReal[i] = analogRead(FFT_INPUT_PIN);
    vImag[i] = 0;

    while (micros() < (newtime + sampling_period)) {
    }
  }

  // FFT
  FFT.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES);

  for (int i = 2; i < (FFT_SAMPLES / 2); i++) {
    vReal[i] = constrain(vReal[i], 0, 65535);
    vReal[i] = map(vReal[i], 0, 65535, 0, 255);
    if (i > 1 && i <= 3) {
      buckets[0] = (vReal[i] > buckets[0]) ? (uint8_t)vReal[i] : buckets[0];
    }
    if (i > 5 && i <= 6) {
      buckets[1] = (vReal[i] > buckets[1]) ? (uint8_t)vReal[i] : buckets[1];
    }
    if (i > 6 && i <= 9) {
      buckets[2] = (vReal[i] > buckets[2]) ? (uint8_t)vReal[i] : buckets[2];
    }
    if (i > 9 && i <= 15) {
      buckets[3] = (vReal[i] > buckets[3]) ? (uint8_t)vReal[i] : buckets[3];
    }
    if (i > 15 && i <= 40) {
      buckets[4] = (vReal[i] > buckets[4]) ? (uint8_t)vReal[i] : buckets[4];
    }
    if (i > 40 && i <= 70) {
      buckets[5] = (vReal[i] > buckets[5]) ? (uint8_t)vReal[i] : buckets[5];
    }
    if (i > 70 && i <= 288) {
      buckets[6] = (vReal[i] > buckets[6]) ? (uint8_t)vReal[i] : buckets[6];
    }
    if (i > 288) {
      buckets[7] = (vReal[i] > buckets[7]) ? (uint8_t)vReal[i] : buckets[7];
    }
  }

  uint32_t colors[FFT_BUCKETS] = {
      CRGB::Red,  CRGB::OrangeRed, CRGB::Orange, CRGB::Green,
      CRGB::Aqua, CRGB::Blue,      CRGB::Violet, CRGB::MediumVioletRed};

  for (int i = 0; i < FFT_BUCKETS; i++) {
    Serial.printf("%i\t", buckets[i]);
    if (buckets[i] < 127) {
      ledset(i * segment, (i * segment) + segment).fadeToBlackBy(2);
    } else {
      ledset(i * segment, (i * segment) + segment) = colors[i];
      ledset(i * segment, (i * segment) + segment).nscale8(buckets[i]);
    }
    buckets[i] = 0;
  }
  Serial.printf("\n");
}

/**
 * eight colored dots, weaving in and out of sync with each other
 */
void Effects::effectJuggle() {
  fadeToBlackBy(leds, numberOfLeds, 20);
  byte dothue = 0;
  for (int i = 0; i < 8; i++) {
    leds[beatsin16(i + 7, 0, numberOfLeds - 1)] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

Effects::Effect Effects::getCurrentEffect() { return currentEffectType; }

void Effects::setStartHue(float hue) {
  startHue = static_cast<uint8_t>(hue * (256.0 / 360.0));
}
