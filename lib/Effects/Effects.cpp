
#include "Effects.h"

#ifdef FFT_ACTIVE
#define SAMPLING_FREQUENCY 40000
double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];

int buckets[FFT_BUCKETS] = {0};
int segment = LED_COUNT / FFT_BUCKETS;

arduinoFFT FFT = arduinoFFT();
#endif

#ifdef MSG_ACTIVE
MD_MSGEQ7 MSGEQ7(MSG_RESET, MSG_STROBE, MSG_DATA);
#endif
// Gradient palette "Paired_07_gp", originally from
// http://soliton.vm.bytemark.co.uk/pub/cpt-city/cb/qual/tn/Paired_07.png.index.html
// converted for FastLED with gammas (2.6, 2.2, 2.5)
// Size: 56 bytes of program space.

DEFINE_GRADIENT_PALETTE(Paired_07_gp){
    0,   83,  159, 190, 36,  83,  159, 190, 36,  1,   48,  106, 72,  1,
    48,  106, 72,  100, 189, 54,  109, 100, 189, 54,  109, 3,   91,  3,
    145, 3,   91,  3,   145, 244, 84,  71,  182, 244, 84,  71,  182, 188,
    1,   1,   218, 188, 1,   1,   218, 249, 135, 31,  255, 249, 135, 31};

CRGBPalette16 vuColors = Paired_07_gp;
Effects::Effects() {
  currentCommand = &Effects::cmdEmpty;
  currentEffect = &Effects::cmdEmpty;

  currentCommandType = Command::None;
  currentEffectType = Effect::NullEffect;

  MSGEQ7.begin();
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
  Serial.printf("  - Effects: Setting new effect: %s\n", effect.c_str());
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
  if (str == "VUMeter") return Effect::VUMeter;
  if (str == "Frequencies") return Effect::Frequencies;
  if (str == "MSGSerial") return Effect::MSGSerial;

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
    case Effect::VUMeter:
      currentEffect = &Effects::effectVUMeter;
      break;
    case Effect::Frequencies:
      currentEffect = &Effects::effectFrequencies;
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
    case Effect::MSGSerial:
      currentEffect = &Effects::effectMSGSerial;
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
  EVERY_N_MILLIS(4) {
    for (uint16_t i = 0; i < N; i++) {
      fadeTowardColor(L[i], bgColor, fadeAmount);
      if (L[i] == bgColor) check++;
    }
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

  fadeTowardColor(leds, numberOfLeds, targetColor, 2);
}

void Effects::setLeds(CRGB* l, const uint16_t& n) {
  numberOfLeds = n;
  leds = l;
}

Effects::Effect Effects::getCurrentEffect() { return currentEffectType; }

void Effects::setStartHue(float hue) {
  startHue = static_cast<uint8_t>(hue * (256.0 / 360.0));
  confettiHue = startHue;
}

// ======================================================================
// FFT Helper functions
// ======================================================================

unsigned long newtime = 0;
unsigned long oldtime = 0;
unsigned long sampling_period = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

unsigned long Effects::sampleDelay() {
  newtime = micros() - oldtime;
  oldtime = newtime;

  return (newtime + sampling_period);
}

void Effects::fftComputeSampleset() {
  // unsigned long start = micros();
  for (int i = 0; i < FFT_SAMPLES; i++) {
    vReal[i] = analogRead(FFT_INPUT_PIN);
    vImag[i] = 0;
  }
  // unsigned long doneSamples = micros();

  FFT.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES);

  // unsigned long doneCompute = micros();
}

void Effects::fftFillBuckets() {
  int maxabove = 0;
  int maxbelow = 0;
  int maxval = 0;

  // ====================================================================
  // 0: Bass: 60 - 250 Hz
  maxval = max((int)vReal[2], maxval);
  maxabove = (int)vReal[6];

  if (maxabove >= (maxval * 0.33)) maxval = 0;

  maxval = constrain(maxval, 0, 16383);
  maxval = map(maxval, 0, 16383, 0, 255);

  buckets[0] = (uint8_t)maxval;

  // ====================================================================
  // 1: Low midrange: 250 - 500 Hz
  maxval = 0;
  maxabove = 0;
  maxbelow = 0;

  for (int i = 5; i < 10; i++) {
    maxval = max((int)vReal[i], maxval);
  }

  maxbelow = (int)vReal[2];

  for (int i = 10; i < 20; i++) {
    maxabove = max((int)vReal[i], maxabove);
  }

  if (maxabove > (maxval * 0.5)) maxval = 0;
  if (maxbelow > (maxval * 1.5)) maxval = 0;
  if (maxval < 6000) maxval = 0;  // remove noise from below.

  maxval = constrain(maxval, 0, 16383);
  maxval = map(maxval, 0, 16383, 0, 255);
  buckets[1] = (uint8_t)maxval;

  // ====================================================================
  // 2: Midrange: 500 - 1 kHz
  maxval = 0;
  maxabove = 0;
  maxbelow = 0;

  for (int i = 3; i < 6; i++) {
    maxbelow = max((int)vReal[i], maxbelow);
  }

  for (int i = 20; i < 40; i++) {
    maxabove = (vReal[i] > maxabove) ? vReal[i] : maxabove;
  }

  for (int i = 10; i < 20; i++) {
    maxval = (vReal[i] > maxval) ? vReal[i] : maxval;
  }
  if (maxabove >= (maxval * 0.40)) maxval = 0;
  if (maxbelow > (maxval * 1.95)) maxval = 0;

  // maxval = maxval - 10000;
  // if (maxval < 0) maxval = 0;
  maxval = constrain(maxval, 0, 16383);
  maxval = map(maxval, 0, 16383, 0, 255);

  buckets[2] = (uint8_t)maxval;

  // ====================================================================
  // 3: Midrange: 1 - 2 kHz
  maxval = 0;
  maxabove = 0;
  maxbelow = 0;

  for (int i = 20; i < 40; i++) {
    maxval = (vReal[i] > maxval) ? vReal[i] : maxval;
  }

  for (int i = 41; i < 80; i++) {
    maxabove = (vReal[i] > maxabove) ? vReal[i] : maxabove;
  }
  if (maxabove >= (maxval * 0.4)) maxval = 0;
  maxval -= 8000;
  if (maxval < 0) maxval = 0;

  maxval = constrain(maxval, 0, 12000);
  maxval = map(maxval, 0, 12000, 0, 255);

  buckets[3] = (uint8_t)maxval;

  // ====================================================================
  // 4: Upper Midrange: 2 - 4 kHz
  maxval = 0;
  maxabove = 0;

  for (int i = 40; i < 80; i++) {
    maxval = (vReal[i] > maxval) ? vReal[i] : maxval;
  }

  for (int i = 80; i < 160; i++) {
    maxabove = (vReal[i] > maxabove) ? vReal[i] : maxabove;
  }
  if (maxabove >= (maxval * 0.4)) maxval = 0;
  maxval -= 8000;
  if (maxval < 0) maxval = 0;

  maxval = constrain(maxval, 0, 12000);
  maxval = map(maxval, 0, 12000, 0, 255);

  buckets[4] = (uint8_t)maxval;

  // ====================================================================
  //   // 5: precence: 4 - 6 kHz
  maxval = 0;
  for (int i = 80; i < 120; i++) {
    maxval = (vReal[i] > maxval) ? vReal[i] : maxval;
  }

  maxabove = 0;
  for (int i = 120; i < 240; i++) {
    maxabove = (vReal[i] > maxabove) ? vReal[i] : maxabove;
  }
  if (maxabove > (maxval * 0.4)) maxval = 0;

  maxval -= 8000;
  if (maxval < 0) maxval = 0;

  maxval = constrain(maxval, 0, 12000);
  maxval = map(maxval, 0, 12000, 0, 255);

  buckets[5] = (uint8_t)maxval;

  // ====================================================================
  // 6: Brilliance: 6 - 20 kHz kHz
  maxval = 0;
  for (int i = 120; i < 240; i++) {
    maxval = (vReal[i] > maxval) ? vReal[i] : maxval;
  }

  maxval = maxval - 10000;
  if (maxval < 0) maxval = 0;

  maxval = constrain(maxval, 0, 9000);
  maxval = map(maxval, 0, 9000, 0, 255);

  buckets[6] = (uint8_t)maxval;
}

// =====================================================================
// EFFECTS
// =====================================================================
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
  EVERY_N_SECONDS(2) { confettiHue = confettiHue + 8; }

  EVERY_N_MILLIS(1000 / FPS) {
    fadeToBlackBy(leds, numberOfLeds, 20);
    int pos = random16(numberOfLeds);
    // leds[pos] += CHSV(startHue + random8(64), 200, 255);
    leds[pos] += CHSV(confettiHue + random8(64), 200, 255);
    // leds[pos] += CHSV(confettiHue, 200, 255);
  }
}

// a colored dot sweeping back and forth, with fading trails
void Effects::effectSinelon() {
  EVERY_N_MILLIS(8) { fadeToBlackBy(leds, numberOfLeds, 32); }
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

/** =====================================================================
 * FFT based spectrum analyzer disco lights
 */
void Effects::effectVUMeter() {
  CRGBSet ledset(leds, LED_COUNT);
  // ledset(0, FFT_BUCKETS * segment) = CRGB::Black;

  EVERY_N_MILLIS(1000 / 25) {
    // Serial.print(micros());
    // Serial.print(" --\t");

    ledset(0, LED_COUNT).fadeLightBy(96);
    // ledset(0, LED_COUNT) = CRGB::Black;
    fftComputeSampleset();
    fftFillBuckets();

    // Colors
    uint32_t colors[FFT_BUCKETS] = {CRGB::Red,   CRGB::OrangeRed, CRGB::Orange,
                                    CRGB::Green, CRGB::Aqua,      CRGB::Blue,
                                    CRGB::Violet};

    // ==================================================================
    // Paint the colors
    for (int i = 0; i < FFT_BUCKETS; i++) {
      // map the value into number of leds to light.
      uint8_t count = map(buckets[i], 0, 255, 0, segment);
      if (count >= 1) {
        ledset(i * segment, (i * segment) + count) = colors[i];
      }
      buckets[i] = 0;
    }
  }
}

void Effects::MusicDancer() {
  EVERY_N_SECONDS(10) { Serial.println("  - effect: music dancer"); }
}

uint8_t freqBuckets[LED_COUNT];
void Effects::effectFrequencies() {
  EVERY_N_SECONDS(10) { Serial.println("  - effect: display frequencies"); }

  CRGBSet ledset(leds, LED_COUNT);

  ledset(0, LED_COUNT) = CRGB::Black;

  for (int i = 0; i < FFT_SAMPLES; i++) {
    // newtime = micros() - oldtime;
    // oldtime = newtime;

    vReal[i] = analogRead(FFT_INPUT_PIN);
    vImag[i] = 0;

    while (micros() < sampleDelay())
      ;
  }

  // FFT
  FFT.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES);

  for (int i = 2; i < (FFT_SAMPLES / 2); i++) {
    vReal[i] = constrain(vReal[i], 0, 65535);
    vReal[i] = map(vReal[i], 0, 65535, 0, 255);

    uint8_t buck = map(i, 0, ((FFT_SAMPLES / 2) - 1), 0, LED_COUNT);
    freqBuckets[buck] =
        (vReal[i] > freqBuckets[buck]) ? (uint8_t)vReal[i] : freqBuckets[buck];
  }

  for (int i = 0; i < LED_COUNT; i++) {
    leds[i].setHSV(startHue, 255, (freqBuckets[i] > 64) ? freqBuckets[i] : 0);
    freqBuckets[i] = 0;
  }
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

void Effects::effectMSGSerial() {
  EVERY_N_SECONDS(2) {
    // msg serial
    // Serial.println("  - effect: MSG Serial");
  }

  //  uint32_t colors[FFT_BUCKETS] = {CRGB::Red,   CRGB::OrangeRed,
  //  CRGB::Orange,
  //                                  CRGB::Green, CRGB::Aqua,      CRGB::Blue,
  //                                  CRGB::Violet};
  //
  CRGBSet ledset(leds, LED_COUNT);
  ledset(0, LED_COUNT) = CRGB::Black;

  EVERY_N_MILLIS(1000 / FPS) {
    MSGEQ7.read();

    CRGBSet ledset(leds, LED_COUNT);
    ledset(0, LED_COUNT) = CRGB::Black;

    for (uint8_t i = 0; i < MAX_BAND; i++) {
      unsigned long value = map(MSGEQ7.get(i), 20, 4095, 0, 255);
      //        Serial.printf("%i\t", (int)value);
      ledset(i * 8, (i * 8) + 8) = CHSV(startHue, 255, value);
    }
    Serial.println();
  }
}