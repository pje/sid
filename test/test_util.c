#include <float.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include "../util.h"

#define EPSILON 0.00001

#define assert_float_eq(expected, actual) { \
  double diff = fabs(expected - actual); \
  if(diff > EPSILON) { \
    printf("%s:%d in %s(): \n\tfailure: expected %lf, got %lf\n", __FILE__, __LINE__, __func__, expected, actual); \
  } else { \
    printf("."); \
  } \
}

int test_sine_waveform() {
  double result = sine_waveform(1.0, 0, 1.0, 0.0);
  assert_float_eq(0.0, result);

  result = sine_waveform(1.0, 0.25, 1.0, 0.0);
  assert_float_eq(1.0, result);

  result = sine_waveform(1.0, 0.5, 1.0, 0.0);
  assert_float_eq(0.0, result);

  result = sine_waveform(1.0, 0.75, 1.0, 0.0);
  assert_float_eq(-1.0, result);

  result = sine_waveform(1.0, 1.0, 1.0, 0.0);
  assert_float_eq(0.0, result);

  return 0;
}

int test_linear_envelope() {
  double result;
  // attack
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 3.0, -1.0);
  assert_float_eq(0.3, result);

  // decay
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 12.5, -1.0);
  assert_float_eq(0.75, result);

  // sustain
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 20, -1.0);
  assert_float_eq(0.5, result);

  // release
  result = linear_envelope(10.0, 5.0, 0.5, 5.0, 20, 2.5);
  assert_float_eq(0.25, result);

  return 0;
}

int main(int argc, char **argv) {
  test_sine_waveform();
  test_linear_envelope();

  printf("\n");

  return 0;
}
