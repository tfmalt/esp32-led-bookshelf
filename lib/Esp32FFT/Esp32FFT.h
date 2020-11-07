#ifndef ESP32FFT_H
#define ESP32FFT_H

#include <Arduino.h>
#include <FastLED.h>
#include <arduinoFFT.h>
#include <driver/adc.h>

#include <array>

#define SAMPLING_FREQUENCY 40000
#define FFT_SAMPLES 1024
#define FFT_BUCKETS 6
#define FFT_LOW_CUTOFF 32000
#define FFT_HIGH_CUTOFF 320000
#define AVG_MAX 200
#define AVG_BASE 2047

class Esp32FFT {
 public:
  Esp32FFT(){};

  void setup() {
#ifdef DEBUG
    Serial.println("  - Running Esp32FFT setup.");
#endif
    analogReadResolution(12);
    analogSetCycles(6);
    analogSetSamples(1);
    analogSetAttenuation(ADC_11db);
  }

  /**
   * Returns the number of frequency buckets used
   */
  const uint8_t getBucketCount() { return FFT_BUCKETS; }

  std::array<uint8_t, FFT_BUCKETS> getSampleSet() {
    fftComputeSampleset();
    fftFillBuckets();

    return buckets;
  }

 private:
  double vReal[FFT_SAMPLES];
  double vImag[FFT_SAMPLES];

  std::array<uint8_t, FFT_BUCKETS> buckets;
  std::array<uint16_t, AVG_MAX> AVG_SAMP;

  uint32_t AVG_CTR = 0;

  uint32_t newtime = 0;
  uint32_t oldtime = 0;

  uint32_t sampling_period = round(1000000 * (1.0 / SAMPLING_FREQUENCY));

  double AMP_FACTOR = 1.00;

  arduinoFFT fft = arduinoFFT();

  uint32_t sampleDelay() {
    newtime = micros() - oldtime;
    oldtime = newtime;

    return (newtime + sampling_period);
  }

  void fftComputeSampleset() {
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

    fft.Windowing(vReal, FFT_SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    fft.Compute(vReal, vImag, FFT_SAMPLES, FFT_FORWARD);
    fft.ComplexToMagnitude(vReal, vImag, FFT_SAMPLES);
    // unsigned long doneCompute = micros();

    EVERY_N_MILLIS(1000) {
      // Calculate average amplitude every 5 seconds.
      uint32_t avg_sum = 0;
      for (int i = 0; i < AVG_MAX; i++) {
        avg_sum += AVG_SAMP[i];
      }

      double avg = (AVG_CTR > 200) ? avg_sum / (double)AVG_MAX
                                   : avg_sum / (double)AVG_CTR;
      AMP_FACTOR = (avg > 0) ? AVG_BASE / avg : 0;

#ifdef DEBUG
      Serial.printf("Average: raw: %6i\t avg: %8.2f\tfac: %8.2f\tadj: %6i\n",
                    sample, avg, AMP_FACTOR, (int)(avg * AMP_FACTOR));
#endif
    }
  }

  void fftFillBuckets() {
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

    if (prev > (curr * 0.47)) curr = 0;

    //   Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
    curr -= FFT_LOW_CUTOFF;
    if (curr < 0) curr = 0;

    curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
    curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);
    buckets[1] = (uint8_t)curr;

    // ====================================================================
    // 2: Midrange: 500 - 2 kHz
    curr = 0;

    for (int i = 6; i < 24; i++) {
      curr = (vReal[i] > curr) ? vReal[i] : curr;
    }

    // Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
    curr -= FFT_LOW_CUTOFF;
    if (curr < 0) curr = 0;

    curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
    curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

    buckets[2] = (uint8_t)curr;

    // ====================================================================
    // 4: Upper Midrange: 2 - 4 kHz
    curr = 0;

    for (int i = 24; i < 48; i++) {
      curr = max((int)vReal[i], curr);
    }

    //  Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
    curr -= FFT_LOW_CUTOFF;
    if (curr < 0) curr = 0;

    curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
    curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

    buckets[3] = (uint8_t)curr;

    // ====================================================================
    //   // 5: precence: 4 - 6 kHz
    curr = 0;

    for (int i = 48; i < 72; i++) {
      curr = max((int)vReal[i], curr);
    }

    //   Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
    curr -= FFT_LOW_CUTOFF;
    if (curr < 0) curr = 0;

    curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
    curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

    buckets[4] = (uint8_t)curr;

    // ====================================================================
    // 6: Brilliance: 6 - 20 kHz kHz
    curr = 0;

    for (int i = 71; i < 240; i++) {
      curr = max((int)vReal[i], curr);
    }

    // Serial.printf("%6i\t%6i\t%6i\t", prev, curr, next);
    curr -= FFT_LOW_CUTOFF;
    if (curr < 0) curr = 0;

    curr = constrain(curr, 0, FFT_HIGH_CUTOFF);
    curr = map(curr, 0, FFT_HIGH_CUTOFF, 0, 255);

    buckets[5] = (uint8_t)curr;

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

  void fftFillBucketsSimple();
};

#endif  // ESP32FFT_H