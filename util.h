#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int word;

byte lowNibble(byte b) { return(b & 0B00001111); }
byte highNibble(byte b) { return((b & 0B11110000) >> 4); }

// y(t) = A * sin(2πft + φ)
//
// returns a double between `-amplitude` and `amplitude`
double sine_waveform(double frequency, double seconds, double amplitude, double phase) {
  double value = amplitude * sin((2 * 3.141592 * frequency * seconds) + phase);
  return value;
}

// returns [0..1]
double linear_envelope(double a, double d, double s, double r, double seconds, double seconds_since_release_start) {
  if (seconds <= a) { // attack
    return(seconds / a);
  } else if (a < seconds && seconds <= (a + d)) { // decay
    return((((s - 1) * (seconds - a)) / d) + 1);
  } else if (seconds_since_release_start > 0) { // release
    return((((-1 * s) * seconds_since_release_start) / r) + s);
  } else { // sustain
    return(s);
  }
}

// there's apparently not widespread agreement on which frequency midi notes
// represent. What we have below is "scientific pitch notation". Ableton, maxmsp
// and garageband use C3, which is shifted an octave lower.
// https://en.wikipedia.org/wiki/Scientific_pitch_notation#See_also
const double TWELFTH_ROOT_OF_TWO = 1.05946309436;
const int base_number = 57;
const int base_freq = 440;

double note_number_to_frequency(byte note) {
  int half_steps_from_base = note - base_number;

  return(base_freq * (pow(TWELFTH_ROOT_OF_TWO, half_steps_from_base)));
}

#endif /* UTIL_H */
