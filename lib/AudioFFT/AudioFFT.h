#ifndef AUDIOFFT_H
#define AUDIOFFT_H

#include <Arduino.h>
#include <Audio.h>
#include <FastLED.h>

#include <array>
#include <list>

#define FFT_SAMPLES 1024
#define FFT_BUCKETS 6

// GUItool: begin automatically generated code
AudioOutputI2S myi2s;
AudioInputAnalog adc1;  // xy=174,747
AudioSynthWaveformSine sine;
AudioAnalyzeFFT1024 afft;  // xy=358.00000762939453,884.0000267028809

AudioConnection patchCord1(adc1, afft);

class AudioFFT {
 public:
  AudioFFT() {
    // AudioMemory(12);
    // afft.windowFunction(AudioWindowHanning1024);
  }

  void setup() {
    AudioMemory(12);
    afft.windowFunction(AudioWindowHanning1024);
  }

  std::array<uint8_t, FFT_BUCKETS> getSampleSet() {
    // fftComputeSampleset();
    fillBuckets();

    EVERY_N_MILLIS(20) { calcMaxValues(); }

    calcVuBuckets();

    return vu_buckets;
  }

 private:
  std::array<float, FFT_BUCKETS> f_buckets;
  std::array<uint8_t, FFT_BUCKETS> vu_buckets;
  std::list<float> max_values[6];

  void fillBuckets() {
    // Bass: 0 - 250 Hz
    f_buckets[0] = afft.read(2, 6);
    // Low Midrange: 250 - 500 Hz
    f_buckets[1] = afft.read(7, 12);
    // Midrange: 500 - 2000 Hz
    f_buckets[2] = afft.read(13, 47);
    // Midrange: 2000 - 4000 Hz
    f_buckets[3] = afft.read(48, 93);
    // Presence: 4000 - 6000 Hz
    f_buckets[4] = afft.read(94, 140);
    // Brilliance: 6000 - 20000 Hz
    f_buckets[5] = afft.read(140, 384);
  }

  void calcMaxValues() {
    for (uint8_t j = 0; j < 6; j++) {
      max_values[j].push_back(f_buckets[j]);

      if (max_values[j].size() > 2048) {
        max_values[j].pop_front();
      }
    }
  }

  void calcVuBuckets() {
    float all_max = 0.000001;

    for (uint8_t i = 0; i < FFT_BUCKETS; i++) {
      float inner_max = 0.000001;

      for (float v : max_values[i]) {
        inner_max = (v > inner_max) ? v : inner_max;
      }

      all_max = (inner_max > all_max) ? inner_max : all_max;

      if (f_buckets[i] > 0.05 && inner_max / all_max > 0.25) {
        float normalize = f_buckets[i] / (inner_max * 0.9);
        normalize = (normalize > 1.0) ? 1.0 : normalize;
        vu_buckets[i] = (uint8_t)255 * normalize;
      } else {
        vu_buckets[i] = 0;
      }
    }
  }
};
#endif  // AUDIOFFT_H