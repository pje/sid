#include "test_helper.h"
#include "../src/sid.h"

void clock_high() { return; };
void clock_low() { return; };
void cs_high() { return; };
void cs_low() { return; };

static void test_sid_transfer() {
  sid_zero_all_registers();

  sid_state_bytes[0] = 0B11111111;

  assert_byte_eq(0B11111111, sid_state_bytes[0]);
}

static void test_sid_set_volume() {
  sid_zero_all_registers();

  sid_set_volume(0B00001111);

  assert_byte_eq(0B00001111, sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME]);
}

static void test_sid_set_waveform() {
  sid_zero_all_registers();
  byte preexisting_low_nibble = 0B00001010;
  // these lowest 4 bits should never change, no matter what we do to a voice's waveform
  sid_transfer((0 * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL, preexisting_low_nibble);
  sid_transfer((1 * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL, preexisting_low_nibble);
  sid_transfer((2 * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL, preexisting_low_nibble);

  for (byte voice = 0; voice < 3; voice++) {
    // test setting a waveform bit to ON for an individual voice
    sid_set_waveform(voice, SID_NOISE, true);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    // waveform bitmasks should be bitwise additive
    sid_set_waveform(voice, SID_SQUARE, true);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE + SID_SQUARE,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    sid_set_waveform(voice, SID_RAMP, true);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE + SID_SQUARE + SID_RAMP,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    sid_set_waveform(voice, SID_TRIANGLE, true);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE + SID_SQUARE + SID_RAMP + SID_TRIANGLE,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    // waveform bitmasks should be bitwise subtractive
    sid_set_waveform(voice, SID_TRIANGLE, false);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE + SID_SQUARE + SID_RAMP,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    sid_set_waveform(voice, SID_RAMP, false);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE + SID_SQUARE,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    sid_set_waveform(voice, SID_SQUARE, false);
    assert_byte_eq(
      preexisting_low_nibble + SID_NOISE,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );

    sid_set_waveform(voice, SID_NOISE, false);
    assert_byte_eq(
      preexisting_low_nibble + 0,
      sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL]
    );
  }
}

static void test_sid_set_ring_mod() {
  printf("\nWARN: sid_set_ring_mod is untested");
}

static void test_sid_set_test() {
  printf("\nWARN: sid_set_test is untested");
}

static void test_sid_set_sync() {
  printf("\nWARN: sid_set_sync is untested");
}

static void test_sid_set_attack() {
  printf("\nWARN: sid_set_attack is untested");
}

static void test_sid_set_decay() {
  printf("\nWARN: sid_set_decay is untested");
}

static void test_sid_set_sustain() {
  printf("\nWARN: sid_set_sustain is untested");
}

static void test_sid_set_release() {
  printf("\nWARN: sid_set_release is untested");
}

static void test_sid_set_pulse_width() {
  printf("\nWARN: sid_set_pulse_width is untested");
}

static void test_sid_set_filter_frequency() {
  printf("\nWARN: sid_set_filter_frequency is untested");
}

static void test_sid_set_filter_resonance() {
  printf("\nWARN: sid_set_filter_resonance is untested");
}

static void test_sid_set_filter() {
  printf("\nWARN: sid_set_filter is untested");
}

static void test_sid_set_filter_mode() {
  printf("\nWARN: sid_set_filter_mode is untested");
}

static void test_sid_set_voice_frequency() {
  printf("\nWARN: sid_set_voice_frequency is untested");
}

static void test_sid_set_gate() {
  printf("\nWARN: sid_set_gate is untested");
}

int main() {
  setvbuf(stdout, NULL, _IONBF, 0); // disable buffering on stdout

  test_sid_transfer();
  test_sid_set_volume();
  test_sid_set_waveform();
  test_sid_set_ring_mod();
  test_sid_set_test();
  test_sid_set_sync();
  test_sid_set_attack();
  test_sid_set_decay();
  test_sid_set_sustain();
  test_sid_set_release();
  test_sid_set_pulse_width();
  test_sid_set_filter_frequency();
  test_sid_set_filter_resonance();
  test_sid_set_filter();
  test_sid_set_filter_mode();
  test_sid_set_voice_frequency();
  test_sid_set_gate();

  printf("\n");

  return TEST_FAILURE_COUNT;
}
