#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#include <math.h>
#include <string.h>
#include <stdio.h>

typedef unsigned char byte;
typedef unsigned int word;

#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif /* constrain */

byte lowNibble(byte b) { return(b & 0B00001111); }
byte highNibble(byte b) { return((b & 0B11110000) >> 4); }

char *dtostrf (double val, signed char width, unsigned char prec, char *sout); // will be defined later in avr/dtostrf.c
extern char *__malloc_heap_start;

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}

void print_byte_in_binary(byte b) {
  static char str[9];
  str[0] = '\0';

  for (int j = 128; j > 0; j >>= 1) {
    strcat(str, ((b & j) == j) ? "1" : "0");
  }

  printf("%s", str);
}

// need this because Arduino's default `sprintf` is broken with float input :(
void float_as_padded_string(char *str, double f, signed char mantissa_chars, signed char decimal_chars, char padding) {
  mantissa_chars = constrain(mantissa_chars, 0, 10);
  decimal_chars = constrain(decimal_chars, 0, 10);
  unsigned int len = mantissa_chars + decimal_chars + 1;

  dtostrf(f, len, decimal_chars, str);

  char format[20];
  sprintf(format, "%%%ds", len);

  sprintf(str, format, str);

  for(unsigned int i = 0; i < len; i++) {
    if (' ' == str[i]) {
      str[i] = padding;
    }
  }
}

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

#endif /* SRC_UTIL_H */
