#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int word;


#define MIN(a,b) \
  ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
     _a <= _b ? _a : _b; })

void reverse(char s[]) {
  int i, j;
  char c;
  for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
    c = s[i];
    s[i] = s[j];
    s[j] = c;
  }
}

void itoa(int n, char s[]) {
  int i, sign;
  if ((sign = n) < 0) {
    n = -n;
  }
  i = 0;
  do {
    s[i++] = n % 10 + '0';
  } while ((n /= 10) > 0);
  if (sign < 0) {
    s[i++] = '-';
  }
  s[i] = '\0';
  reverse(s);
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

byte lowNibble(byte b) { return(b & 0B00001111); }
byte highNibble(byte b) { return((b & 0B11110000) >> 4); }

#endif /* UTIL_H */
