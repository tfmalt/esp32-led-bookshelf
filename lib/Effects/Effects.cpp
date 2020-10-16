
#include "Effects.h"

#ifdef FFT_ACTIVE
#define SAMPLING_FREQUENCY 40000
#define FFT_SAMPLES 1024
#define FFT_BUCKETS 7
double vReal[FFT_SAMPLES];
double vImag[FFT_SAMPLES];

int buckets[FFT_BUCKETS] = {0};
int segment = LED_COUNT / FFT_BUCKETS;

arduinoFFT FFT = arduinoFFT();
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

  fill_solid(leds, LED_COUNT, CRGB::Black);
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
  if (str == "Music Dancer") return Effect::MusicDancer;
  if (str == "Pride") return Effect::Pride;

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
    case Effect::Pride:
      currentEffect = &Effects::effectPride;
      break;
    case Effect::VUMeter:
      currentEffect = &Effects::effectVUMeter;
      break;
    case Effect::MusicDancer:
      currentEffect = &Effects::effectMusicDancer;
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

// 5 per second for 20 seconds

#define AVG_MAX 200
#define AVG_BASE 2047
#define FFT_LOW_CUTOFF 32000
#define FFT_HIGH_CUTOFF 320000

uint16_t AVG_SAMP[AVG_MAX];
uint32_t AVG_CTR = 0;
double AMP_FACTOR = 1.00;

void Effects::fftComputeSampleset() {
  // unsigned long start = micros();
  uint16_t sample;

  for (int i = 0; i < FFT_SAMPLES; i++) {
    sample = analogRead(FFT_INPUT_PIN);

    EVERY_N_MILLIS(200) {
      AVG_SAMP[(AVG_CTR % AVG_MAX)] = sample;
      AVG_CTR++;
    }

    int adjusted = (int)(sample * AMP_FACTOR);
    if (adjusted < 0) adjusted = 0;
    if (adjusted > 4095) adjusted = 4095;

    vReal[i] = adjusted;
    vImag[i] = 0;
  }
  // unsigned long doneSamples = micros();

  FFT.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES);
  // unsigned long doneCompute = micros();

  EVERY_N_MILLIS(10000) {
    // Calculate average amplitude every 5 seconds.
    uint32_t avg_sum = 0;
    for (int i = 0; i < AVG_MAX; i++) {
      avg_sum += AVG_SAMP[i];
    }

    double avg =
        (AVG_CTR > 200) ? avg_sum / (double)AVG_MAX : avg_sum / (double)AVG_CTR;
    AMP_FACTOR = (avg > 0) ? AVG_BASE / avg : 0;

#ifdef DEBUG
    Serial.printf("Average: raw: %6i\t avg: %8.2f\tfac: %8.2f\tadj: %6i\n",
                  sample, avg, AMP_FACTOR, (int)(avg * AMP_FACTOR));
#endif
  }
}

void Effects::fftFillBuckets() {
  int next = 0;
  int prev = 0;
  int curr = 0;

  // ====================================================================
  // 0: Bass: 60 - 250 Hz
  curr = (int)vReal[2];
  next = (int)vReal[3];

  if (next > (curr * 2.19)) curr = 0;

  //  Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);

  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[0] = (uint8_t)curr;

  // ====================================================================
  // 1: Low midrange: 250 - 500 Hz
  curr = 0;
  next = 0;
  prev = (int)vReal[2];

  for (int i = 3; i < 6; i++) {
    curr = max((int)vReal[i], curr);
  }

  //   for (int i = 6; i < 11; i++) {
  //     next = max((int)vReal[i], next);
  //   }

  if (prev > (curr * 0.47)) curr = 0;
  // if (next > (curr * 0.4)) curr = 0;

  //   Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);
  buckets[1] = (uint8_t)curr;

  // ====================================================================
  // 2: Midrange: 500 - 1 kHz
  // prev = 0;
  curr = 0;
  // next = 0;

  // for (int i = 2; i < 6; i++) {
  //   prev = max((int)vReal[i], prev);
  // }
  //
  for (int i = 6; i < 12; i++) {
    curr = (vReal[i] > curr) ? vReal[i] : curr;
  }
  //
  //   for (int i = 11; i < 21; i++) {
  //     next = (vReal[i] > next) ? vReal[i] : next;
  //   }
  //
  //   if (next > (curr * 0.36)) curr = 0;
  //   if (prev > (curr * 2.68)) curr = 0;

  // Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[2] = (uint8_t)curr;

  // ====================================================================
  // 3: Midrange: 1 - 2 kHz
  curr = 0;
  // next = 0;
  // prev = 0;
  //
  //   for (int i = 2; i < 11; i++) {
  //     prev = max((int)vReal[i], prev);
  //   }
  //
  for (int i = 12; i < 24; i++) {
    curr = max((int)vReal[i], curr);
  }
  //
  //   for (int i = 21; i < 41; i++) {
  //     next = max((int)vReal[i], next);
  //   }
  //
  //   if (next > (curr * 0.29)) curr = 0;
  //   if (prev > (curr * 2.84)) curr = 0;

  //  Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[3] = (uint8_t)curr;

  // ====================================================================
  // 4: Upper Midrange: 2 - 4 kHz
  curr = 0;
  // next = 0;
  //
  //   for (int i = 11; i < 21; i++) {
  //     prev = max((int)vReal[i], prev);
  //   }
  //
  for (int i = 24; i < 48; i++) {
    curr = max((int)vReal[i], curr);
  }
  //
  //   for (int i = 41; i < 80; i++) {
  //     next = max((int)vReal[i], next);
  //   }
  //
  //   if (prev > (curr * 3.74)) curr = 0;
  //   if (next > (curr * 0.30)) curr = 0;

  //  Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[4] = (uint8_t)curr;

  // ====================================================================
  //   // 5: precence: 4 - 6 kHz
  curr = 0;
  // next = 0;
  //
  //   for (int i = 21; i < 40; i++) {
  //     prev = max((int)vReal[i], prev);
  //   }
  //
  for (int i = 48; i < 72; i++) {
    curr = max((int)vReal[i], curr);
  }
  //
  //   for (int i = 60; i < 240; i++) {
  //     next = max((int)vReal[i], next);
  //   }
  //
  //   if (prev > (curr * 1.10)) curr = 0;
  //   if (next > (curr * 0.86)) curr = 0;

  //   Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[5] = (uint8_t)curr;

  // ====================================================================
  // 6: Brilliance: 6 - 20 kHz kHz
  curr = 0;
  // next = 0;
  //
  //   for (int i = 40; i < 60; i++) {
  //     prev = max((int)vReal[i], prev);
  //   }
  //
  for (int i = 71; i < 240; i++) {
    curr = max((int)vReal[i], curr);
  }
  //
  //   if (prev > (curr * 1.30)) curr = 0;

  // Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
  curr -= FFT_LOW_CUTOFF;
  if (curr < 0) curr = 0;

  curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
  curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

  buckets[6] = (uint8_t)curr;

  // ====================================================================
  // Debug printing:
  // ====================================================================
  // int FROM = 70;
  // int TO = 175;
  // int MOD = 2;
  //
  //   for (int i = FROM; i < TO; i++) {
  //     if (i % MOD == 0) Serial.printf("%6i\t", (int)vReal[i]);
  //   }
  //   Serial.println();
  //
  //   EVERY_N_MILLIS(1000) {
  //     Serial.println();
  //     Serial.printf("%6s\t%6s\t%6s\t", "prev", "curr", "next");
  //
  //     for (int i = FROM; i < TO; i++) {
  //       if (i % MOD == 0) Serial.printf("%6i\t", i);
  //     }
  //     Serial.println();
  //     for (int i = FROM; i < (TO + 3); i++) {
  //       if (i % MOD == 0) Serial.printf("------\t");
  //     }
  //     Serial.println();
  //   }
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

void Effects::effectPride() {
  EVERY_N_SECONDS(2) { Serial.println("  - Pride colors"); }
  CRGBSet ledset(leds, LED_COUNT);

  uint8_t num_colors = 6;
  uint8_t segment = LED_COUNT / num_colors;

  uint32_t colors[num_colors] = {0xEF0000, 0xFF4000, 0xFFEF00,
                                 0x008000, 0x0000F0, 0x800070};

  for (int i = 0; i < num_colors; i++) {
    ledset(i * segment, (i * segment) + segment) = colors[i];
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

    ledset(0, LED_COUNT).fadeToBlackBy(96);
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

void Effects::effectMusicDancer() {
  CRGBSet ledset(leds, LED_COUNT);
  // ledset(0, LED_COUNT) = CRGB::Black;

  fftComputeSampleset();
  fftFillBuckets();

  uint16_t middle = LED_COUNT / 2;

  uint16_t bass_size = LED_COUNT / 6;
  uint16_t mid_size = LED_COUNT / 3;
  uint16_t low_size = LED_COUNT;

  uint16_t bass_amp = map(buckets[0], 0, 255, 0, ((LED_COUNT / 6) * 2));
  uint16_t mid_amp = map(buckets[2], 0, 255, 0, ((LED_COUNT / 3) * 2));
  uint16_t low_amp = map(buckets[3], 0, 255, 0, (LED_COUNT / 2));
  uint16_t high_amp = map(buckets[4], 0, 255, 0, (LED_COUNT / 2));
  uint16_t bril_amp = map(buckets[5], 0, 255, 0, (LED_COUNT / 2));
  uint16_t air_amp = map(buckets[6], 0, 255, 0, (LED_COUNT / 2));

  uint16_t bass_start = middle - bass_size;
  uint16_t bass_stop = middle + bass_size;

  uint16_t mid_start = middle - mid_size;
  uint16_t mid_stop = middle + mid_size;

  ledset.fadeToBlackBy(48);
  // ledset.fadeLightBy(48);

  for (int i = 0; i < high_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    leds[pos] = CHSV(96 + random8(16), 255, buckets[4]);
  }

  for (int i = 0; i < bril_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    leds[pos] = CHSV(128 + random8(16), 255, buckets[5]);
  }

  for (int i = 0; i < air_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    leds[pos] = CHSV(192 + random8(16), 128, buckets[6]);
  }

  for (int i = 0; i < low_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    leds[pos] = CHSV(144 + random8(16), 255, buckets[3]);
  }

  for (int i = 0; i < mid_amp; i++) {
    uint16_t pos = random16(mid_start, mid_stop);
    leds[pos] = CHSV(24 + random8(32), 255, buckets[2]);
  }

  for (int i = 0; i < bass_amp; i++) {
    uint16_t pos = random16(bass_start, bass_stop);
    leds[pos] = CHSV(248 + random8(16), 255, buckets[0]);
  }
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
  EVERY_N_MILLIS(1000 / FPS) {
    fadeToBlackBy(leds, numberOfLeds, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, numberOfLeds - 1)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
}