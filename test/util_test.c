#include "test_helper.h"
#include "../util.h"

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
  double result = 0;
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

int test_freqs() {
  assert_float_eq(16.351598,   note_number_to_frequency(0));
  assert_float_eq(440.0,       note_number_to_frequency(57));
  assert_float_eq(3951.066410, note_number_to_frequency(95));

  return 0;
}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0); // disable buffering on stdout

  test_sine_waveform();
  test_linear_envelope();
  test_freqs();

  printf("\n");
  return 0;
}
