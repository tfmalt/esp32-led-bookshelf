
#include "Effects.hpp"

#if defined(FFT_ACTIVE) && defined(IS_TEENSY)
#include <AudioFFT.h>
AudioFFT fft;
#endif

#if defined(FFT_ACTIVE) && defined(IS_ESP32)
#include <Esp32FFT.h>
Esp32FFT fft;
#endif

DEFINE_GRADIENT_PALETTE(Paired_07_gp){
    0,   83,  159, 190, 36,  83,  159, 190, 36,  1,   48,  106, 72,  1,
    48,  106, 72,  100, 189, 54,  109, 100, 189, 54,  109, 3,   91,  3,
    145, 3,   91,  3,   145, 244, 84,  71,  182, 244, 84,  71,  182, 188,
    1,   1,   218, 188, 1,   1,   218, 249, 135, 31,  255, 249, 135, 31};

DEFINE_GRADIENT_PALETTE(bhw1_05_gp){0, 1, 221, 53, 255, 73, 3, 178};
DEFINE_GRADIENT_PALETTE(RdYlBu_gp){
    0,   82, 0,   2,   31,  163, 6,   2,   63,  227, 39,  9,  95,  249,
    109, 22, 127, 252, 191, 61,  127, 182, 229, 237, 159, 90, 178, 203,
    191, 32, 108, 155, 223, 8,   45,  106, 255, 3,   8,   66};

DEFINE_GRADIENT_PALETTE(Sunset_Real_gp){
    0,  120, 0,   0,   22, 179, 22,  0,  51, 255, 104, 0, 85, 167,
    22, 18,  135, 100, 0,  103, 198, 16, 0,  130, 255, 0, 0,  160};

DEFINE_GRADIENT_PALETTE(summer_gp){
    0,   0,   55,  25, 17,  1,   62,  25, 33,  1,   72,  25, 51,  3,   82,  25,
    68,  8,   92,  25, 84,  14,  104, 25, 102, 23,  115, 25, 119, 35,  127, 25,
    135, 48,  141, 25, 153, 67,  156, 25, 170, 88,  169, 25, 186, 112, 186, 25,
    204, 142, 201, 25, 221, 175, 217, 25, 237, 210, 235, 25, 255, 255, 255, 25};

DEFINE_GRADIENT_PALETTE(gr65_hult_gp){0,   247, 176, 247, 48,  255, 136, 255,
                                      89,  220, 29,  226, 160, 7,   82,  178,
                                      216, 1,   124, 109, 255, 1,   124, 109};

using namespace Effects;

void Effects::Controller::setup(CRGB* l, const uint16_t n,
                                LightState::LightState s) {
#ifdef DEBUG
  Serial.println("[effects] running setup.");
#endif

  numberOfLeds = n;
  leds = l;
  state = s;

#if defined(FFT_ACTIVE) && defined(IS_ESP32)
  fft.setup();
#endif

  setInitialState();
}

void Effects::Controller::setInitialState() {
  Serial.printf("[effects] setting initial state: '%s'\n",
                state.state ? "On" : "Off");

  CRGBSet ledset(leds, numberOfLeds);

  if (state.state == false) {
    FastLED.setBrightness(0);
    return;
  }

  Serial.printf(
      "[effects] color: r: %i, g: %i, b: %i, x: %.2f, y: %.2f, h: %.2f, s: "
      "%.2f\n",
      state.color.r, state.color.g, state.color.b, state.color.x, state.color.y,
      state.color.h, state.color.s);
  // ledset = CRGB(state.color.r, state.color.g, state.color.b);
  setCurrentCommand(Effects::Command::Color);

  Serial.printf("[effects] effect: '%s'\n", state.effect.c_str());
  setCurrentEffect(state.effect);

  Serial.printf("[effects] brightness: %i\n", state.brightness);
  setCurrentCommand(Effects::Command::Brightness);
}

/**
 * Handle an incoming state update
 */
void Effects::Controller::handleStateChange(LightState::LightState s) {
  state = s;
  Serial.printf("[effects] got state change: %s\n", state.state ? "ON" : "OFF");

  if (state.state == false) {
    FastLED.setBrightness(0);
    return;
  }

  if (state.status.hasEffect) {
    setCurrentEffect(state.effect);

    if (state.effect == "") {
      setCurrentCommand(Effects::Command::Color);
    }
  }

  if (state.status.hasBrightness) {
#ifdef DEBUG
    Serial.printf("[effects]   Got new brightness: '%i'\n", state.brightness);
#endif
    setCurrentCommand(Effects::Command::Brightness);
  }

  if (state.status.hasColor) {
#ifdef DEBUG
    Serial.println("[effects]   Got color");
#endif
    if (getCurrentEffect() == Effects::Effect::NullEffect) {
      setCurrentCommand(Effects::Command::Color);
    } else {
#ifdef DEBUG
      Serial.printf("[effects]   effect is: '%s' hue: %.2f\n",
                    state.effect.c_str(), state.color.h);
#endif
      setStartHue(state.color.h);
    }
  }

  if (state.status.hasColorTemp) {
    unsigned int kelvin = (1000000 / state.color_temp);
#ifdef DEBUG
    Serial.printf("[effects]   Got color temp: %i mired = %i Kelvin\n",
                  state.color_temp, kelvin);
#endif

    unsigned int temp = kelvin / 100;

    double red = 0;
    if (temp <= 66) {
      red = 255;
    } else {
      red = temp - 60;
      red = 329.698727446 * (pow(red, -0.1332047592));
      if (red < 0) red = 0;
      if (red > 255) red = 255;
    }

    double green = 0;
    if (temp <= 66) {
      green = temp;
      green = 99.4708025861 * log(green) - 161.1195681661;
      if (green < 0) green = 0;
      if (green > 255) green = 255;
    } else {
      green = temp - 60;
      green = 288.1221695283 * (pow(green, -0.0755148492));
      if (green < 0) green = 0;
      if (green > 255) green = 255;
    }

    double blue = 0;
    if (temp >= 66) {
      blue = 255;
    } else {
      if (temp <= 19) {
        blue = 0;
      } else {
        blue = temp - 10;
        blue = 138.5177312231 * log(blue) - 305.0447927307;
        if (blue < 0) blue = 0;
        if (blue > 255) blue = 255;
      }
    }

#ifdef DEBUG
    Serial.printf("[effects]   RGB [%i, %i, %i]\n", static_cast<uint8_t>(red),
                  static_cast<uint8_t>(green), static_cast<uint8_t>(blue));
#endif
    state.color.r = static_cast<uint8_t>(red);
    state.color.g = static_cast<uint8_t>(green);
    state.color.b = static_cast<uint8_t>(blue);

    setCurrentCommand(Effects::Command::Color);
  }

  if (!state.status.hasColorTemp && !state.status.hasColor &&
      !state.status.hasBrightness && !state.status.hasEffect) {
    // assuming turn on is the only thing.
    setCurrentCommand(Effects::Command::Brightness);
    setCurrentCommand(Effects::Command::Color);
  }
}

// void Effects::Controller::setLightStateController(LightState::Controller* l)
// {
//   lightState = l;
// }

void Effects::Controller::setCommandFrames(uint16_t i) { commandFrames = i; }

void Effects::Controller::setCurrentCommand(Command cmd) {
  // LightState::LightState& state = lightState->getCurrentState();
  commandFrameCount = 0;
  commandStart = millis();
  currentCommandType = cmd;

  setCommandFrames(FPS * (state.transition || 1));

  switch (cmd) {
    case Command::Brightness:
      cmdQueue[Command::Brightness] = [this]() { cmdSetBrightness(); };
      break;
    case Command::Color:
      cmdQueue[Command::Color] = [this]() { cmdFadeTowardColor(); };
      break;
    case Command::FirmwareUpdate:
      cmdQueue[Command::FirmwareUpdate] = [this]() { cmdFirmwareUpdate(); };
      break;
    default:
      // currentCommand = &Effects::Controller::cmdEmpty;
      currentCommandType = Command::None;
  }
}

Effects::Effect Effects::Controller::getEffectFromString(std::string str) {
  if (str == "Glitter Rainbow") return Effect::GlitterRainbow;
  if (str == "Rainbow") return Effect::Rainbow;
  if (str == "Gradient") return Effect::Gradient;
  if (str == "RainbowByShelf") return Effect::RainbowByShelf;
  if (str == "BPM") return Effect::BPM;
  if (str == "Confetti") return Effect::Confetti;
  if (str == "Juggle") return Effect::Juggle;
  if (str == "Sinelon") return Effect::Sinelon;
  if (str == "VUMeter") return Effect::VUMeter;
  if (str == "Frequencies") return Effect::Frequencies;
  if (str == "Music Dancer") return Effect::MusicDancer;
  if (str == "Pride") return Effect::Pride;
  if (str == "Colorloop") return Effect::Colorloop;
  if (str == "Walking Rainbow") return Effect::WalkingRainbow;

  return Effect::NullEffect;
}

void Effects::Controller::setCurrentEffect(std::string effect) {
#ifdef DEBUG
  Serial.printf("[effects] setting effect: %s\n", effect.c_str());
#endif

  fill_solid(leds, LED_COUNT, CRGB::Black);
  setCurrentEffect(getEffectFromString(effect));
}

void Effects::Controller::setCurrentEffect(Effect effect) {
  // LightState::LightState& state = lightState->getCurrentState();
  currentEffectType = effect;

  switch (effect) {
    case Effect::GlitterRainbow:
      currentEffect = [this]() { effectGlitterRainbow(); };
      break;
    case Effect::Rainbow:
      currentEffect = [this]() { effectRainbow(); };
      break;
    case Effect::Gradient:
      currentEffect = [this]() { effectGradient(); };
      break;
    case Effect::RainbowByShelf:
      currentEffect = [this]() { effectRainbowByShelf(); };
      break;
    case Effect::BPM:
      currentEffect = [this]() { effectBPM(); };
      break;
    case Effect::Pride:
      currentEffect = [this]() { effectPride(); };
      break;
    case Effect::Colorloop:
      currentEffect = [this]() { effectColorloop(); };
      break;
    case Effect::WalkingRainbow:
      currentEffect = [this]() { effectWalkingRainbow(); };
      break;
    case Effect::VUMeter:
      currentEffect = [this]() { effectVUMeter(); };
      break;
    case Effect::MusicDancer:
      currentEffect = [this]() { effectMusicDancer(); };
      break;
    case Effect::Frequencies:
      currentEffect = [this]() { effectFrequencies(); };
      break;
    case Effect::Confetti:
      currentEffect = [this]() { effectConfetti(); };
      break;
    case Effect::Sinelon:
      currentEffect = [this]() { effectSinelon(); };
      break;
    case Effect::Juggle:
      currentEffect = [this]() { effectJuggle(); };
      break;
    default:
      currentEffectType = Effect::NullEffect;
      currentEffect = [this]() {};
  }
}

void Effects::Controller::runCurrentCommand() {
  for (auto func : cmdQueue) {
    func.second();
  }
}

void Effects::Controller::runCurrentEffect() { this->currentEffect(); }

void Effects::Controller::cmdEmpty() {}

void Effects::Controller::cmdFirmwareUpdate() {
  fill_solid(leds, LED_COUNT, CRGB::Black);
  // fill_solid(leds, 15, CRGB::White);
  leds[3] = CRGB::White;
  leds[7] = CRGB::White;
  leds[11] = CRGB::White;
  leds[15] = CRGB::White;
  leds[16] = CRGB::White;

  FastLED.show();
}

void Effects::Controller::cmdSetBrightness() {
  // LightState::LightState state = lightState->getCurrentState();

  uint8_t target = state.brightness;
  uint8_t current = FastLED.getBrightness();

  EVERY_N_MILLIS(1000 / FPS) {
    if (commandFrameCount < commandFrames) {
      FastLED.setBrightness(
          current + ((target - current) / (commandFrames - commandFrameCount)));
      commandFrameCount++;
    } else {
#ifdef DEBUG
      Serial.printf("[effects] command setting brightness DONE [%i] %lu ms.\n",
                    FastLED.getBrightness(), (millis() - commandStart));
#endif

      // setCurrentCommand(Command::None);
      cmdQueue.erase(Effects::Command::Brightness);
    }
  }
}

/**
 * Helper function that blends one uint8_t toward another by a given amount
 * Taken from:
 * https://gist.github.com/kriegsman/d0a5ed3c8f38c64adcb4837dafb6e690
 */
void Effects::Controller::nblendU8TowardU8(uint8_t& cur, const uint8_t target,
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
CRGB Effects::Controller::fadeTowardColor(CRGB& cur, const CRGB& target,
                                          uint8_t amount) {
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
void Effects::Controller::fadeTowardColor(CRGB* L, uint16_t N,
                                          const CRGB& bgColor,
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
    Serial.printf("[effects] fade towards color done in %lu ms.\n",
                  (millis() - commandStart));
#endif
    // setCurrentCommand(Command::None);
    cmdQueue.erase(Effects::Command::Color);
  }
}

/**
 * command tells strip to Fade towards a color
 */
void Effects::Controller::cmdFadeTowardColor() {
  // LightState::LightState state = lightState->getCurrentState();
  CRGB targetColor(state.color.r, state.color.g, state.color.b);

  fadeTowardColor(leds, numberOfLeds, targetColor, 2);
}

/**
 * Return current effect
 *
 * @return Effects::Effect current effect type
 */
Effects::Effect Effects::Controller::getCurrentEffect() {
  // returns current effect
  return currentEffectType;
}

/**
 * Sets the start hue
 */
void Effects::Controller::setStartHue(float hue) {
  startHue = static_cast<uint8_t>(hue * (256.0 / 360.0));
  confettiHue = startHue;
}

// =====================================================================
// EFFECTS
// =====================================================================
void Effects::Controller::effectRainbow() {
  // fills the leds with rainbow colors
  fill_rainbow(leds, numberOfLeds, startHue, 2);
}

uint16_t GRAD_INDEX = 0;
CRGBPalette256 pal = Sunset_Real_gp;
uint8_t j = 0;

void Effects::Controller::effectGradient() {
  CRGBPalette256 palettes[6] = {RdYlBu_gp, Paired_07_gp, bhw1_05_gp,
                                summer_gp, gr65_hult_gp, Sunset_Real_gp};

  EVERY_N_SECONDS(30) {
    Serial.printf("  - effect Gradient #: %i\n", j);
    pal = palettes[j];
    j = (j < 5) ? j + 1 : 0;
  }

  uint8_t step = (256 / LED_COUNT);

  // EVERY_N_MILLIS(1000 / FPS) { GRAD_INDEX++; }
  EVERY_N_MILLIS(1000 / FPS) {
    GRAD_INDEX = beatsin16(2, 0, 257 * 8);
    fill_palette(leds, LED_COUNT, GRAD_INDEX, step, pal, 255, LINEARBLEND);
    // fill_palette(end, (LED_COUNT / 2) - 1, REV_INDEX, step, pal, 255,
    //              LINEARBLEND);
    // end = start;
  }
}

void Effects::Controller::effectRainbowByShelf() {
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

void Effects::Controller::addGlitter(fract8 chanceOfGlitter) {
  if (random8() < chanceOfGlitter) {
    leds[random16(numberOfLeds)] += CRGB::White;
  }
}

void Effects::Controller::effectGlitterRainbow() {
  // built-in FastLED rainbow, plus some random sparkly glitter
  effectRainbow();
  EVERY_N_MILLIS(1000 / FPS) { addGlitter(160); }
}

void Effects::Controller::effectConfetti() {
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
void Effects::Controller::effectSinelon() {
  EVERY_N_MILLIS(8) { fadeToBlackBy(leds, LED_COUNT, 16); }

  EVERY_N_MILLIS(200) { startHue += 1; }
  // LightState state = lightState->getCurrentState();

  // calculate a suiting pulserate for the number of leds.
  uint8_t bpm = 750 / LED_COUNT;

  int pos = beatsin16(bpm, 0, LED_COUNT - 1);
  leds[pos] = CHSV(startHue, 255, 255);
  // leds[pos] += CRGB(state.color.r, state.color.g, state.color.b);
}

/**
 * colored stripes pulsing at a defined Beats-Per-Minute (BPM)
 */
void Effects::Controller::effectBPM() {
  uint8_t BeatsPerMinute = 64;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8(BeatsPerMinute, 64, 255);
  for (int i = 0; i < numberOfLeds; i++) {  // 9948
    leds[i] = ColorFromPalette(palette, startHue + (i * 2),
                               beat - startHue + (i * 10));
  }
}

void Effects::Controller::effectPride() {
  CRGBSet ledset(leds, LED_COUNT);

  uint8_t num_colors = 6;
  uint8_t segment = LED_COUNT / num_colors;

  uint32_t colors[num_colors] = {0xEF0000, 0xFF4000, 0xFFEF00,
                                 0x008000, 0x0000F0, 0x800070};

  for (int i = 0; i < num_colors; i++) {
    ledset(i * segment, (i * segment) + segment) = colors[i];
  }
}

void Effects::Controller::effectWalkingRainbow() {
  EVERY_N_MILLIS(1000 / 60) {
    uint8_t inc = 2;  // 256 / LED_COUNT;
    uint8_t hue = startHue;

    for (int i = 0; i < LED_COUNT; i++) {
      hue += inc;
      leds[i] = CHSV(hue, 255, 255);
    }

    startHue--;
  }
}

void Effects::Controller::effectColorloop() {
  // EVERY_N_SECONDS(2) { Serial.printf("  - Running colorloop: %i\n",
  // startHue); }

  EVERY_N_MILLIS(1000 / 10) {
    CRGBSet ledset(leds, LED_COUNT);
    startHue += 1;
    ledset = CHSV(startHue, 255, 255);
  }
}

/** =====================================================================
 * FFT based spectrum analyzer disco lights
 */
void Effects::Controller::effectVUMeter() {
  CRGBSet ledset(leds, LED_COUNT);

  EVERY_N_MILLIS(1000 / 25) {
    ledset(0, LED_COUNT).fadeToBlackBy(96);
    std::array<uint8_t, FFT_BUCKETS> buckets = fft.getSampleSet();

    // ==================================================================
    // Paint the colors
    CRGBPalette16 palette = Rainbow_gp;
    uint8_t segment = LED_COUNT / FFT_BUCKETS;  // how many leds per bucket
    uint8_t step = 256 / FFT_BUCKETS;    // How many colors to jump per segment
    uint8_t increment = step / segment;  // colors to increment inside segment

    for (int i = 0; i < FFT_BUCKETS; i++) {
      // map the value into number of leds to light.
      uint8_t count = map(buckets[i], 0, 255, 0, segment);

      if (count > 0) {
        fill_palette(ledset(i * segment, i * segment + count), count, i * step,
                     increment, palette, buckets[i], LINEARBLEND);
      }
      // buckets[i] = 0;
    }
  }
}

CRGBPalette256 colPal = Sunset_Real_gp;
void Effects::Controller::effectMusicDancer() {
  CRGBSet ledset(leds, LED_COUNT);
  // ledset(0, LED_COUNT) = CRGB::Black;

  std::array<uint8_t, FFT_BUCKETS> buckets = fft.getSampleSet();
  //   fftComputeSampleset();
  //   fftFillBuckets();

  uint16_t middle = LED_COUNT / 2;

  uint16_t bass_size = LED_COUNT / 10;
  uint16_t mid_size = LED_COUNT / 10;
  // uint16_t low_size = LED_COUNT / 10;

  uint16_t bass_amp = map(buckets[0], 0, 255, 0, ((LED_COUNT / 10) * 2));
  uint16_t mid_amp = map(buckets[2], 0, 255, 0, ((LED_COUNT / 10) * 2));
  uint16_t low_amp = map(buckets[3], 0, 255, 0, (LED_COUNT / 2));
  uint16_t high_amp = map(buckets[4], 0, 255, 0, (LED_COUNT / 2));
  uint16_t bril_amp = map(buckets[5], 0, 255, 0, LED_COUNT);
  uint16_t air_amp = map(buckets[6], 0, 255, 0, LED_COUNT);

  uint16_t bass_start = middle - bass_size;
  uint16_t bass_stop = middle + bass_size;

  uint16_t mid_start = bass_start - mid_size;
  uint16_t mid_stop = bass_stop + mid_size;

  ledset.fadeToBlackBy(48);
  // ledset.fadeLightBy(48);

  // CRGBPalette16 colPal = bhw1_05_gp;
  // CRGBPalette16 colPal = Paired_07_gp;  // bhw1_05_gp
  // CRGBPalette16 colPal = Rainbow_gp;  // bhw1_05_gp
  // CRGBPalette256 colPal = Sunset_Real_gp;
  CRGBPalette256 palettes[6] = {RdYlBu_gp, Paired_07_gp, bhw1_05_gp,
                                summer_gp, gr65_hult_gp, Sunset_Real_gp};

  EVERY_N_SECONDS(30) {
    Serial.printf("  - effect Gradient #: %i\n", j);
    colPal = palettes[j];
    j = (j < 5) ? j + 1 : 0;
  }

  uint8_t RAND = 32;

  for (int i = 0; i < high_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    // leds[pos] = CHSV(96 + random8(16), 255, buckets[4]);
    leds[pos] = ColorFromPalette(colPal, 223 + random8(RAND));
  }

  for (int i = 0; i < bril_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    // leds[pos] = CHSV(128 + random8(16), 255, buckets[5]);
    leds[pos] = ColorFromPalette(colPal, 192 + random8(RAND));
  }

  for (int i = 0; i < air_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    // leds[pos] = CHSV(192 + random8(16), 128, buckets[6]);
    leds[pos] = ColorFromPalette(colPal, 144 + random8(RAND));
  }

  for (int i = 0; i < low_amp; i++) {
    uint16_t pos = random16(LED_COUNT);
    leds[pos] = ColorFromPalette(colPal, 96 + random8(RAND));
    // leds[pos] = CHSV(144 + random8(16), 255, buckets[3]);
  }

  for (int i = 0; i < mid_amp; i++) {
    uint16_t pos = random16(mid_start, mid_stop);
    // leds[pos] = CHSV(24 + random8(32), 255, buckets[2]);
    leds[pos] = ColorFromPalette(colPal, 48 + random8(RAND));
  }

  for (int i = 0; i < bass_amp; i++) {
    uint16_t pos = random16(bass_start, bass_stop);
    // leds[pos] = CHSV(248 + random8(16), 255, buckets[0]);
    leds[pos] = ColorFromPalette(colPal, 0 + random8(RAND));
  }
}

uint8_t freqBuckets[LED_COUNT];
void Effects::Controller::effectFrequencies() {
  EVERY_N_SECONDS(10) { Serial.println("  - effect: display frequencies"); }

  CRGBSet ledset(leds, LED_COUNT);

  ledset(0, LED_COUNT) = CRGB::Black;
}

/**
 * eight colored dots, weaving in and out of sync with each other
 */
void Effects::Controller::effectJuggle() {
  EVERY_N_MILLIS(1000 / FPS) {
    fadeToBlackBy(leds, numberOfLeds, 20);
    byte dothue = 0;
    for (int i = 0; i < 8; i++) {
      leds[beatsin16(i + 7, 0, numberOfLeds - 1)] |= CHSV(dothue, 200, 255);
      dothue += 32;
    }
  }
}