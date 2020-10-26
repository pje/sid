#include "Arduino.h"
#include <avr/io.h>
#include <math.h>
#include <usbmidi.h>
#include "src/deque.h"
#include "src/hash_table.h"
#include "src/note.h"
#include "src/stdinout.h"
#include "src/util.h"

#define DEBUG_LOGGING true

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

const unsigned int deque_size = 32; // the number of notes that can be held simultaneously
deque *notes = deque_initialize(deque_size, stdout, _note_indexer, _note_node_print_function);

const int ARDUINO_SID_CHIP_SELECT_PIN = 13; // wired to SID's CS pin
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5; // wired to SID's Ø2 pin

const float CLOCK_SIGNAL_FACTOR = 0.0596;

const byte SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO   = 0;
const byte SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI   = 1;
const byte SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO = 2;
const byte SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI = 3;
const byte SID_REGISTER_OFFSET_VOICE_CONTROL        = 4;
const byte SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD    = 5;
const byte SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR    = 6;

const byte SID_REGISTER_ADDRESS_FILTER_FREQUENCY_LO      = 21;
const byte SID_REGISTER_ADDRESS_FILTER_FREQUENCY_HI      = 22;
const byte SID_REGISTER_ADDRESS_FILTER_RESONANCE         = 23;
const byte SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME       = 24;

const byte SID_NOISE         = 0B10000000;
const byte SID_SQUARE        = 0B01000000;
const byte SID_RAMP          = 0B00100000;
const byte SID_TRIANGLE      = 0B00010000;
const byte SID_TEST          = 0B00001000;
const byte SID_RING          = 0B00000100;
const byte SID_SYNC          = 0B00000010;
const byte SID_GATE          = 0B00000001;
const byte SID_3OFF          = 0B10000000;
const byte SID_FILTER_HP     = 0B01000000;
const byte SID_FILTER_BP     = 0B00100000;
const byte SID_FILTER_LP     = 0B00010000;
const byte SID_FILTER_OFF    = 0B00000000;
const byte SID_FILTER_VOICE1 = 0B00000001;
const byte SID_FILTER_VOICE2 = 0B00000010;
const byte SID_FILTER_VOICE3 = 0B00000100;
const byte SID_FILTER_EXT    = 0B00001000;

const float SID_MIN_OSCILLATOR_HERTZ = 16.35;
const float SID_MAX_OSCILLATOR_HERTZ = 3951.06;

// since we have to set all the bits in a register byte at once,
// we must maintain a copy of the register's state so we don't clobber bits
// (SID actually has 28 registers but we don't use the last 4. hence 25)
byte sid_state_bytes[25] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const byte MIDI_NOTE_ON        = 0B1001;
const byte MIDI_NOTE_OFF       = 0B1000;
const byte MIDI_PITCH_BEND     = 0B1110;
const byte MIDI_CONTROL_CHANGE = 0B1011;
const byte MIDI_PROGRAM_CHANGE = 0B1100;
const byte MIDI_TIMING_CLOCK   = 0B11111000;

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_TRIANGLE   = 12; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_RAMP       = 13; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_SQUARE     = 14; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_NOISE      = 15; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE            = 16; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE                = 17; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_ONE                = 82; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_ONE              = 18; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE         = 38; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_ONE     = 70; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_ONE              = 39; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE               = 40; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE             = 41; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE             = 42; // 4-bit value

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_TRIANGLE   = 20; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_RAMP       = 21; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_SQUARE     = 22; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_NOISE      = 23; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO            = 24; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO                = 25; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_TWO                = 90; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_TWO              = 26; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO         = 46; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_TWO     = 78; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO              = 47; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO               = 48; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO             = 49; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO             = 50; // 4-bit value

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_TRIANGLE = 28; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_RAMP     = 29; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_SQUARE   = 30; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_NOISE    = 31; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE          = 33; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE              = 34; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_THREE              = 98; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE            = 35; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE       = 54; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_THREE   = 86; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE            = 55; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE             = 56; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE           = 57; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE           = 58; // 4-bit value

const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY              = 36; // 7-bit value (11-bit total)
const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY_LSB          = 68; // 4-bit value (11-bit total)
const byte MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE              = 37; // 4-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_LP             = 60; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_BP             = 61; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_HP             = 62; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE_OFF        = 63; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE              = 64; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO              = 72; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE            = 80; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_VOLUME                        = 43; // 4-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_SID_TRANSFER_DEBUGGING     = 119; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME_LSB                = 124; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME                    = 125; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_TOGGLE_ALL_TEST_BITS              = 126; // 1-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_VOLUME_MODULATION_MODE     = 81; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_PULSE_WIDTH_MODULATION_MODE= 83; // 1-bit value

const byte MIDI_CONTROL_CHANGE_RPN_MSB                           = 101;
const byte MIDI_CONTROL_CHANGE_RPN_LSB                           = 100;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY                        = 6;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY_FINE                   = 38;
const byte MIDI_RPN_PITCH_BEND_SENSITIVITY                       = 0;
const byte MIDI_RPN_MASTER_FINE_TUNING                           = 1;
const byte MIDI_RPN_MASTER_COARSE_TUNING                         = 2;
const word MIDI_RPN_NULL                                         = 16383;

// below are CC numbers that are defined and we don't implement, but repurposing
// them in the future might have weird consequences, so we shouldn't
const byte MIDI_CONTROL_CHANGE_ALL_NOTES_OFF                     = 123;

const byte MIDI_CHANNEL = 0; // "channel 1" (zero-indexed)
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_PARAPHONIC        = 0;
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC        = 1;
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC_LEGATO = 2;
const byte MIDI_PROGRAM_CHANGE_HARDWARE_RESET                    = 127;

const byte MAX_POLYPHONY = 3;
const byte DEFAULT_PITCH_BEND_SEMITONES = 5;
const double DEFAULT_GLIDE_TIME_MILLIS = 100.0;
const unsigned int GLIDE_TIME_MIN_MILLIS = 1;
const unsigned int GLIDE_TIME_MAX_MILLIS = 10000;
const word MAX_PULSE_WIDTH_VALUE = 4095;

byte polyphony = 3;
bool legato_mode = false;
float glide_time_millis = DEFAULT_GLIDE_TIME_MILLIS;
word glide_time_raw_word;
byte glide_time_raw_lsb;
unsigned long glide_start_time_micros = 0;
byte glide_to = 0;
byte glide_from = 0;

struct note oscillator_notes[3] = { { .number=0, .on_time=0, .off_time=0 } };

int midi_pitch_bend_max_semitones = DEFAULT_PITCH_BEND_SEMITONES;
double current_pitchbend_amount = 0.0; // [-1.0 .. 1.0]
double voice_detune_percents[MAX_POLYPHONY] = { 0.0, 0.0, 0.0 }; // [-1.0 .. 1.0]
int detune_max_semitones = 5;

const word DEFAULT_PULSE_WIDTH = 2048; // (2**12 - 1) / 2
const byte DEFAULT_WAVEFORM = SID_TRIANGLE;
const byte DEFAULT_ATTACK = 0;
const byte DEFAULT_DECAY = 0;
const byte DEFAULT_SUSTAIN = 15;
const byte DEFAULT_RELEASE = 0;
const unsigned short int DEFAULT_FILTER_FREQUENCY = 1000;
const byte DEFAULT_FILTER_RESONANCE = 15;
const byte DEFAULT_VOLUME = 15;

// experimental: used to implement 14-bit resolution for PW values spread over two sequential CC messages
word pw_v1     = 0;
byte pw_v1_lsb = 0;
word pw_v2     = 0;
byte pw_v2_lsb = 0;
word pw_v3     = 0;
byte pw_v3_lsb = 0;

word filter_frequency = 0;
byte filter_frequency_lsb = 0;

// used to implement RPN messages
word rpn_value = 0;
word data_entry = 0;

// You know that click when we change the SID's volume? Turns out if we modulate
// that click we can generate arbitrary 4-bit wveforms including sine waves and
// sample playback. (Currently this mode just means "4-bit sine")
bool volume_modulation_mode_active = false;

bool pulse_width_modulation_mode_active = false;
const unsigned int PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY = 65535;

unsigned long last_update = 0;
unsigned long last_glide_update_micros = 0;
const float update_every_micros = (100.0 / 4.41);

unsigned long time_in_micros = 0;
unsigned long time_in_seconds = 0;

static void __deque_inspect_nodes(node *n) {
  if (n == NULL) {
    return;
  }
  _note_node_print_function(n, stdout);
  if (n->next) {
    Serial.print("-");
    __deque_inspect_nodes(n->next);
  } else {
    return;
  }
}

const float sid_attack_values_to_seconds[16] = {
  0.002,
  0.008,
  0.016,
  0.024,
  0.038,
  0.056,
  0.068,
  0.080,
  0.100,
  0.250,
  0.500,
  0.800,
  1.000,
  3.000,
  5.000,
  8.000
};

const float sid_decay_and_release_values_to_seconds[16] = {
  0.006,
  0.024,
  0.048,
  0.072,
  0.114,
  0.168,
  0.204,
  0.240,
  0.300,
  0.750,
  1.500,
  2.400,
  3.000,
  9.000,
  15.00,
  24.00
};

void clock_high() {
  uint8_t oldSREG = SREG;
  cli();

  PORTC |= 0B01000000;
  // digitalWrite(ARDUINO_SID_MASTER_CLOCK_PIN, HIGH);

  SREG = oldSREG;
}

void clock_low() {
  uint8_t oldSREG = SREG;
  cli();

  PORTC &= 0B10111111;
  // digitalWrite(ARDUINO_SID_MASTER_CLOCK_PIN, LOW);

  SREG = oldSREG;
}

void cs_high() {
  uint8_t oldSREG = SREG;
  cli();

  PORTC |= 0B10000000;
  // digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  SREG = oldSREG;
}

void cs_low() {
  uint8_t oldSREG = SREG;
  cli();

  PORTC &= 0B01111111;
  // digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, LOW);

  SREG = oldSREG;
}

word get_voice_frequency_register_value(unsigned int voice) {
  word value = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI];
  value <<= 8;
  value += sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO];
  return value;
}

double get_voice_frequency(unsigned int voice) {
  word frequency = get_voice_frequency_register_value(voice);
  double hertz = frequency * CLOCK_SIGNAL_FACTOR;
  return(hertz);
}

word get_voice_pulse_width(unsigned int voice) {
  word pulse_width = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI];
  pulse_width <<= 8;
  pulse_width += sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO];
  return(pulse_width);
}

byte get_voice_waveform(unsigned int voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & 0B11110000) >> 4);
}

bool get_voice_test_bit(unsigned int voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_TEST) != 0);
}

bool get_voice_ring_mod(unsigned int voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_RING) != 0);
}

bool get_voice_sync(unsigned int voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_SYNC) != 0);
}

bool get_voice_gate(unsigned int voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_GATE) != 0);
}

double get_attack_seconds(unsigned int voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_attack_values_to_seconds[value]);
}

double get_decay_seconds(unsigned int voice) {
  byte value = lowNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_decay_and_release_values_to_seconds[value]);
}

// returns float [0..1]
double get_sustain_percent(unsigned int voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR]);
  return((double) value / 15.0);
}

double get_release_seconds(unsigned int voice) {
  byte value = lowNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR]);
  return(sid_decay_and_release_values_to_seconds[value]);
}

word get_filter_frequency() {
  word frequency = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_FREQUENCY_HI];
  frequency <<= 3;
  frequency += sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_FREQUENCY_LO];
  return(frequency);
}

byte get_filter_resonance() {
  byte resonance = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_RESONANCE] & B11110000;
  resonance >>= 4;
  return(resonance);
}

byte get_volume() {
  return(sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME] & B00001111);
}

bool get_filter_enabled_for_voice(unsigned int voice) {
  byte bits = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_RESONANCE] & B00000111;
  bits >>= voice;
  bits <<= (8 - (voice + 1));
  return(bits != 0);
}

void sid_transfer(byte address, byte data) {
  address &= 0B00011111;

  // optimization: don't send anything if SID already has that data in that register
  if (sid_state_bytes[address] == data) {
    return;
  }

  // PORTF is a weird 6-bit register (8 bits, but bits 2 and 3 don't exist)
  //
  // Port F Data Register — PORTF
  // bit  7    6    5    4    3    2    1    0
  //      F7   F6   F5   F4   -    -    F1   F0
  //
  // addr -    A4   A3   A2   -    -    A1   A0

  byte data_for_port_f = ((address << 2) & 0B01110000) | (address & 0B00000011);

  noInterrupts();

  clock_high();
  clock_low();

  PORTF = data_for_port_f;
  PORTB = data;

  cs_low();
  clock_high();

  clock_low();
  cs_high();

  interrupts();

  sid_state_bytes[address] = data;
}

void sid_zero_all_registers() {
  for (byte i = 0; i < 25; i++) {
    sid_transfer(i, 0B00000000);
  }
}

void sid_set_volume(byte level) {
  byte address = SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME;
  byte data = (sid_state_bytes[address] & 0B11110000) | (level & 0B00001111);
  sid_transfer(address, data);
}

void sid_set_waveform(int voice, byte waveform) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = (waveform | (sid_state_bytes[address] & 0B00001111));
  sid_transfer(address, data);
}

void sid_set_ring_mod(int voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = 0;
  if (on) {
    // ring mod repurposes the output of the triangle oscillator
    data = sid_state_bytes[address] | 0B00010100; // set triangle and ring mod bits, leave others as-is
  } else {
    if (((sid_state_bytes[address] >> 7) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_NOISE);
    } else if (((sid_state_bytes[address] >> 6) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_SQUARE);
    } else if (((sid_state_bytes[address] >> 5) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_RAMP);
    } else {
      sid_set_waveform(voice, SID_TRIANGLE);
    }
    data = sid_state_bytes[address] & 0B11111011; // zero ring mod
  }
  sid_transfer(address, data);
}

void sid_set_test(int voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = (on ? (sid_state_bytes[address] | SID_TEST) : (sid_state_bytes[address] & ~SID_TEST));
  sid_transfer(address, data);
}

void sid_set_sync(int voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = (on ? (sid_state_bytes[address] | SID_SYNC) : (sid_state_bytes[address] & ~SID_SYNC));
  sid_transfer(address, data);
}

void sid_set_attack(int voice, byte attack) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & 0B00001111;
  data |= (attack << 4);
  sid_transfer(address, data);
}

void sid_set_decay(int voice, byte decay) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & 0B11110000;
  data |= (decay & 0B00001111);
  sid_transfer(address, data);
}

void sid_set_sustain(int voice, byte sustain) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & 0B00001111;
  data |= (sustain << 4);
  sid_transfer(address, data);
}

void sid_set_release(int voice, byte release) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & 0B11110000;
  data |= (release & 0B00001111);
  sid_transfer(address, data);
}

void sid_set_pulse_width(byte voice, word hertz) { // 12-bit value
  byte lo = lowByte(hertz);
  byte hi = highByte(hertz) & 0B00001111;
  byte address_lo = (voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO;
  byte address_hi = (voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI;
  sid_transfer(address_lo, lo);
  sid_transfer(address_hi, hi);
}

void sid_set_filter_frequency(word hertz) { // 11-bit value
  byte lo = lowByte(hertz) & 0B00000111;
  byte hi = highByte(hertz << 5);
  sid_transfer(SID_REGISTER_ADDRESS_FILTER_FREQUENCY_LO, lo);
  sid_transfer(SID_REGISTER_ADDRESS_FILTER_FREQUENCY_HI, hi);
}

void sid_set_filter_resonance(byte amount) {
  byte address = SID_REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data = (sid_state_bytes[address] & 0B00001111) | (amount << 4);
  sid_transfer(address, data);
}

void sid_set_filter(byte voice, bool on) {
  voice = constrain(voice, 0, 3); // allow `3` to mean "ext filt on/off"

  byte address = SID_REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data = 0;
  byte voice_filter_mask = 0;

  if (voice == 0) {
    voice_filter_mask = SID_FILTER_VOICE1;
  } else if(voice == 1) {
    voice_filter_mask = SID_FILTER_VOICE2;
  } else if(voice == 2) {
    voice_filter_mask = SID_FILTER_VOICE3;
  } else if(voice == 3) { // allow `3` to mean "ext filt on/off"
    voice_filter_mask = SID_FILTER_EXT;
  } else {
    return;
  }

  if (on) {
    data = sid_state_bytes[address] | voice_filter_mask;
  } else {
    data = sid_state_bytes[address] & ~voice_filter_mask;
  }
  sid_transfer(address, data);
}

// filter modes are additive (e.g. you can set LP & HP simultaneously)
//
// const byte SID_FILTER_HP     = 0B01000000;
// const byte SID_FILTER_BP     = 0B00100000;
// const byte SID_FILTER_LP     = 0B00010000;
void sid_set_filter_mode(byte mode, bool on) {
  byte address = SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME;
  byte data = sid_state_bytes[address];
  if (on) {
    data |= mode;
  } else {
    data &= ~mode;
  }
  sid_transfer(address, data);
}

void sid_set_voice_frequency(int voice, double hertz) {
  word frequency = (hertz / CLOCK_SIGNAL_FACTOR);
  byte loFrequency = lowByte(frequency);
  byte hiFrequency = highByte(frequency);

  // optimization: if the voice's frequency hasn't changed, don't send it
  word prev = get_voice_frequency_register_value(voice);
  if (hiFrequency != highByte(prev)) {
    byte hiAddress = (voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI;
    sid_transfer(hiAddress, hiFrequency);
  }

  if (loFrequency != lowByte(prev)) {
    byte loAddress = (voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO;
    sid_transfer(loAddress, loFrequency);
  }
}

void sid_set_gate(int voice, bool state) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data;
  if (state) {
    data = sid_state_bytes[address] | SID_GATE;
  } else {
    data = sid_state_bytes[address] & ~SID_GATE;
  }
  sid_transfer(address, data);
}

// SID requires a 1MHz clock signal, so this sets `ARDUINO_SID_MASTER_CLOCK_PIN`
// to be our 1MHz oscillator by configuring Timer 3 of the ATmega32U4.
// http://medesign.seas.upenn.edu/index.php/Guides/MaEvArM-timer3
void start_clock() {
  TCCR3A = 0;
  TCCR3B = 0;
  TCNT3 = 0;
  OCR3A = 7;
  TCCR3A |= (1 << COM3A0);
  TCCR3B |= (1 << WGM32);
  TCCR3B |= (1 << CS30);
}

void nullify_notes_playing() {
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    oscillator_notes[i] = { .number = 0, .on_time = 0, .off_time = 0 };
  }

  deque_empty(notes);
}

void handle_voice_attack_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_attack(i, envelope_value);
    }
  } else {
    sid_set_attack(voice, envelope_value);
  }
}

void handle_voice_decay_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_decay(i, envelope_value);
    }
  } else {
    sid_set_decay(voice, envelope_value);
  }
}

void handle_voice_sustain_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_sustain(i, envelope_value);
    }
  } else {
    sid_set_sustain(voice, envelope_value);
  }
}

void handle_voice_release_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_release(i, envelope_value);
    }
  } else {
    sid_set_release(voice, envelope_value);
  }
}

void handle_voice_waveform_change(byte voice, byte waveform, bool on) {
  if (on) {
    if (polyphony > 1) {
      for (int i = 0; i < 3; i++) {
        sid_set_waveform(i, waveform);
      }
    } else {
      sid_set_waveform(voice, waveform);
    }
  } else { // we turn the voice "off" by zeroing its waveform bits
    if (polyphony > 1) {
      for (int i = 0; i < 3; i++) {
        sid_set_waveform(i, 0);
      }
    } else {
      sid_set_waveform(voice, 0);
    }
  }
}

void handle_voice_filter_change(byte voice, bool on) {
  if (polyphony > 1) {
    byte address = SID_REGISTER_ADDRESS_FILTER_RESONANCE;
    byte data = 0;
    byte voice_filter_mask = (SID_FILTER_VOICE1 | SID_FILTER_VOICE2 | SID_FILTER_VOICE3);

    if (on) {
      data = sid_state_bytes[address] | voice_filter_mask;
    } else {
      data = sid_state_bytes[address] & ~voice_filter_mask;
    }

    printf("handle_voice_filter_change: ");
    print_byte_in_binary(data);
    printf("\n");

    sid_transfer(address, data);
  } else {
    sid_set_filter(voice, on);
  }
}

void handle_voice_pulse_width_change(byte voice, word frequency) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_pulse_width(i, frequency);
    }
  } else {
    sid_set_pulse_width(voice, frequency);
  }
}

void handle_voice_ring_mod_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_ring_mod(i, on);
    }
  } else {
    sid_set_ring_mod(voice, on);
  }
}

void handle_voice_sync_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_sync(i, on);
    }
  } else {
    sid_set_sync(voice, on);
  }
}

void handle_voice_test_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_test(i, on);
    }
  } else {
    sid_set_test(voice, on);
  }
}

void handle_voice_detune_change(byte voice, double detune_factor) {
  voice_detune_percents[voice] = detune_factor;
  double semitone_change = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[voice] * detune_max_semitones);
  double frequency = note_number_to_frequency(oscillator_notes[voice].number) * pow(2, semitone_change / 12.0);
  sid_set_voice_frequency(voice, frequency);
}

// set the oscillator frequency and gate it!
// - takes stateful stuff into account, like pitch bend and detune
// - will first de-gate the oscillator iff it's not already de-gated for some reason
// - handles global modulation modes (if we're in volume mod mode, we don't actually interact with the voice.)
void play_note_for_voice(byte note_number, unsigned char voice) {
  unsigned long now = micros();
  double hertz = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[voice] * detune_max_semitones);
  hertz = note_number_to_frequency(note_number) * pow(2, hertz / 12.0);

  if (!volume_modulation_mode_active) {
    if (get_voice_gate(voice)) {
      if (legato_mode) {
        glide_start_time_micros = now;
        glide_to = note_number;
        glide_from = oscillator_notes[voice].number;
      } else {
        sid_set_gate(voice, false);
      }
    }

    if (pulse_width_modulation_mode_active) {
      sid_set_voice_frequency(voice, PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY);
    } else {
      if (!legato_mode || (oscillator_notes[voice].number == 0 || oscillator_notes[voice].off_time != 0)) { // i.e. no other note is being voiced OR the note being voiced is in its release phase
        sid_set_voice_frequency(voice, hertz);
      }
    }

    if (!get_voice_gate(voice)) {
      sid_set_gate(voice, true);
      oscillator_notes[voice].on_time = now;
    }
  }

  deque_append(notes, { .number=note_number, .on_time=now, .off_time=0, .voiced_by_oscillator=voice });

  oscillator_notes[voice].number = note_number;
  oscillator_notes[voice].off_time = 0;
}

void log_load_stats() {
  printf(
    "{free_mem: %d, h(%u/%u){load: %d%%, coll: %d%%}\n",
    freeMemory(),
    notes->ht->size,
    notes->ht->max_size,
    int(hash_table_load_factor(notes->ht) * 100),
    int(hash_table_collision_ratio(notes->ht) * 100)
  );
}


void inspect_oscillator_notes() {
  printf("{%d, %d, %d}\n", oscillator_notes[0].number, oscillator_notes[1].number, oscillator_notes[2].number);
}

void inspect_voice_detunes() {
  printf(
    "{%d%%, %d%%, %d%%}\n",
    (signed int)(voice_detune_percents[0] * 100),
    (signed int)(voice_detune_percents[1] * 100),
    (signed int)(voice_detune_percents[2] * 100)
  );
}

void handle_note_on(byte note_number) {
  #if DEBUG_LOGGING
    log_load_stats();
    printf(" %s(%d): ", __func__, note_number);
    inspect_oscillator_notes();
  #endif

  // We're mono, so play the same base note on all 3 oscillators
  if (polyphony == 1) {
    for (int i = 0; i < MAX_POLYPHONY; i++ ) {
      if (get_voice_waveform(i) != 0) { // don't even try to play "muted" voices
        play_note_for_voice(note_number, i);
      }
    }
    return;
  }

  // we're poly, so every voice has its own frequency.
  // if there's a free voice, we can just use that.
  for (int i = 0; i < polyphony; i++) {
    if (oscillator_notes[i].number == 0) {
      play_note_for_voice(note_number, i);
      return;
    }
  }

  unsigned int oldest_voice = notes->first->data.voiced_by_oscillator;

  #if DEBUG_LOGGING
    printf("oldest_voice: %d\n", oldest_voice);
  #endif
  play_note_for_voice(note_number, oldest_voice);
}

void handle_note_off(byte note_number) {
  unsigned long now = micros();

  #if DEBUG_LOGGING
    log_load_stats();
    printf("%s(%d) ", __func__, note_number);
    inspect_oscillator_notes();
  #endif

  note *note_from_deque = deque_find_by_key(notes, note_number);
  if (note_from_deque) {
    note_from_deque->off_time = now;
  } else {
    #if DEBUG_LOGGING
      Serial.print("NOTE: received note_off message for an unknown note.");
    #endif
  }

  for (unsigned char i = 0; i < MAX_POLYPHONY; i++) {
    // if the note is being voiced, we just need to start its release phase
    if (oscillator_notes[i].number == note_number) {
      if (pulse_width_modulation_mode_active) {
        oscillator_notes[i].off_time = now;
        continue;
      }
      node *other_most_recent_node = deque_find_node_by_key(notes, note_number)->previous;
      if (legato_mode && other_most_recent_node) {
        // this means more than one note is being held. So we start gliding to the other most recent note. This is how "hammer-off" glides work
        byte new_num = other_most_recent_node->data.number;
        oscillator_notes[i] = { .number=new_num, .on_time=now, .off_time=0, .voiced_by_oscillator=i };
        glide_start_time_micros = now;
        glide_to = new_num;
        glide_from = note_number;
        deque_remove_by_key(notes, note_number);
      } else {
        sid_set_gate(i, false);
        oscillator_notes[i].off_time = now;
      }
    } else {
      // if the note is not being voiced, we may as well try to remove its entry from the deque now
      deque_remove_by_key(notes, note_number);
    }
  }

  if (note_number == glide_to) {
    glide_from = 0;
    glide_to = 0;
  }
}

void handle_pitchbend_change(word pitchbend) {
  double temp_double = 0.0;
  current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the "neutral" pitchbend value (half of 2**14)
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    if (oscillator_notes[i].number != 0) {
      temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      temp_double = note_number_to_frequency(oscillator_notes[i].number) * pow(2, temp_double / 12.0);
      if (!pulse_width_modulation_mode_active){
        sid_set_voice_frequency(i, temp_double);
      }
    }
  }
}

void duplicate_voice(unsigned int from_voice, unsigned int to_voice) {
  for (int i = 0; i < 7; i++) {
    sid_transfer((to_voice * 7) + i, sid_state_bytes[(from_voice * 7) + i]);
  }

  voice_detune_percents[to_voice] = voice_detune_percents[from_voice];
}

void handle_program_change(byte program_number) {
  switch (program_number) {
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_PARAPHONIC:
    polyphony = 3;
    sid_set_gate(0, false);
    sid_set_gate(1, false);
    sid_set_gate(2, false);
    duplicate_voice(0, 1);
    duplicate_voice(0, 2);
    for (int i = 0; i < 3; i++) {
      oscillator_notes[i] = { .number = 0, .on_time = 0, .off_time = 0 };
    }
    legato_mode = false;
    break;
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC:
    polyphony = 1;
    legato_mode = false;
    break;
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC_LEGATO:
    polyphony = 1;
    legato_mode = true;
    break;
  case MIDI_PROGRAM_CHANGE_HARDWARE_RESET:
    clean_slate();
  }
}

void enable_pulse_width_modulation_mode() {
  pulse_width_modulation_mode_active = true;

  for (int i = 0; i < 3; i++) {
    sid_set_waveform(i, SID_SQUARE);
    sid_set_voice_frequency(i, PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY);
  }
}

void disable_pulse_width_modulation_mode() {
  pulse_width_modulation_mode_active = false;

  for (int i = 0; i < 3; i++) {
    sid_set_test(i, false);
    sid_set_waveform(i, SID_TRIANGLE);
  }
}

// need this because Arduino's default `sprintf` is broken with float input :(
void float_as_padded_string(char *str, double f, signed char mantissa_chars, signed char decimal_chars, char padding) {
  mantissa_chars = constrain(mantissa_chars, 0, 10);
  decimal_chars = constrain(decimal_chars, 0, 10);
  size_t len = mantissa_chars + decimal_chars + 1;

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

void handle_state_dump_request(bool human) {
  // for testing purposes, print the state of our sid representation, which
  // should (hopefully) mirror what the SID's registers are right now

  if (human) {
    #if DEBUG_LOGGING
      printf("V#  WAVE     FREQ    A      D      S    R      PW   TEST RING SYNC GATE FILT\n");
      //           "V1  Triangle 1234.56 12.345 12.345 12.345 12.345 2048 1    1    1    1    1   \n"
      for (int i = 0; i < 3; i++) {
        Serial.print("V");
        Serial.print(i);
        Serial.print("  ");

        byte wave = get_voice_waveform(i);
        if (wave == 0) {
          Serial.print("Disabled");
        } else if (wave == 1) {
          Serial.print("Triangle");
        } else if (wave == 2) {
          Serial.print("Ramp    ");
        } else if (wave == 4) {
          Serial.print("Pulse   ");
        } else if (wave == 8) {
          Serial.print("Noise   ");
        }

        char float_string[] = "       ";

        double f = get_voice_frequency(i);

        Serial.print(" ");
        float_as_padded_string(float_string, f, 4, 2, '0');
        Serial.print(float_string);

        double a = get_attack_seconds(i);
        double d = get_decay_seconds(i);
        double s = get_sustain_percent(i);
        double r = get_release_seconds(i);

        Serial.print(" ");
        float_as_padded_string(float_string, a, 2, 3, '0');
        Serial.print(float_string);

        Serial.print(" ");
        float_as_padded_string(float_string, d, 2, 3, '0');
        Serial.print(float_string);

        Serial.print(" ");
        sprintf(float_string, "%3d", (int)(s * 100));
        Serial.print(float_string);
        Serial.print("%");

        Serial.print(" ");
        float_as_padded_string(float_string, r, 2, 3, '0');
        Serial.print(float_string);

        word pw = get_voice_pulse_width(i);

        Serial.print(" ");
        float_as_padded_string(float_string, pw, 0, 4, '0');
        Serial.print(float_string);

        Serial.print(" ");
        Serial.print(get_voice_test_bit(i));
        Serial.print("    ");
        Serial.print(get_voice_ring_mod(i));
        Serial.print("    ");
        Serial.print(get_voice_sync(i));
        Serial.print("    ");
        Serial.print(get_voice_gate(i));
        Serial.print("    ");
        Serial.print(get_filter_enabled_for_voice(i));
        Serial.print("\n");
      }

      Serial.print("Filter frequency: ");
      Serial.print(get_filter_frequency());
      Serial.print(" resonance: ");
      Serial.print(get_filter_resonance());
      Serial.print(" mode: ");

      byte mask = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME] & 0B01110000;

      Serial.print(mask & 0B01000000 ? "HP" : "--");
      Serial.print(mask & 0B00100000 ? "BP" : "--");
      Serial.print(mask & 0B00010000 ? "LP" : "--");
      Serial.print("\n");

      Serial.print("Global Mode: ");

      if (polyphony == 1 && legato_mode) {
        Serial.print("Mono Legato");
      } else if (polyphony == 1) {
        Serial.print("Mono");
      } else {
        Serial.print("Poly");
      }
      if (volume_modulation_mode_active) {
        Serial.print(" <volume modulation mode>");
      } else if (pulse_width_modulation_mode_active) {
        Serial.print(" <pulse width modulation mode>");
      }
      Serial.print("\n");

      Serial.print("Volume: ");
      Serial.print(get_volume());
      Serial.print("\n");
    #endif

    inspect_oscillator_notes();
    deque_inspect(notes);
    log_load_stats();
  } else {
    for (int i = 0; i < 25; i++) {
      print_byte_in_binary(sid_state_bytes[i]);
    }

    log_load_stats();
  }
}

void handle_midi_input(Stream *midi_port) {
  if (midi_port->available() > 0) {
    byte incomingByte = midi_port->read();
    byte opcode  = incomingByte >> 4;
    byte channel = incomingByte & (0B00001111);
    byte data_byte_one = 0;
    byte data_byte_two = 0;
    word pitchbend = 8192.0;
    byte controller_number = 0;
    byte controller_value = 0;

    if (channel == MIDI_CHANNEL && opcode >= 0B1000 && opcode <= 0B1110) { // Voice/Mode Messages, on our channel
      switch (opcode) {
      case MIDI_CONTROL_CHANGE:
        while (midi_port->available() <= 0) {}
        controller_number = midi_port->read();
        while (midi_port->available() <= 0) {}
        controller_value = midi_port->read();

        #if DEBUG_LOGGING
          Serial.print("\n");
          Serial.print("[");
          Serial.print(time_in_micros);
          Serial.print("] ");
          Serial.print("Received MIDI CC ");
          Serial.print(controller_number);
          Serial.print(" ");
          Serial.print(controller_value);
          Serial.print("\n");
        #endif

        switch (controller_number) {
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_SQUARE:
          handle_voice_waveform_change(0, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_SQUARE:
          handle_voice_waveform_change(1, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_SQUARE:
          handle_voice_waveform_change(2, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_TRIANGLE:
          handle_voice_waveform_change(0, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_TRIANGLE:
          handle_voice_waveform_change(1, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_TRIANGLE:
          handle_voice_waveform_change(2, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_RAMP:
          handle_voice_waveform_change(0, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_RAMP:
          handle_voice_waveform_change(1, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_RAMP:
          handle_voice_waveform_change(2, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_NOISE:
          handle_voice_waveform_change(0, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_NOISE:
          handle_voice_waveform_change(1, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_NOISE:
          handle_voice_waveform_change(2, SID_NOISE, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE:
          // replaces the triangle output of voice 1 with a ring modulated combination of voice 1 by voice 3
          handle_voice_ring_mod_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO:
          // replaces the triangle output of voice 2 with a ring modulated combination of voice 2 by voice 1
          handle_voice_ring_mod_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE:
          // replaces the triangle output of voice 3 with a ring modulated combination of voice 3 by voice 2
          handle_voice_ring_mod_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE:
          // hard-syncs frequency of voice 1 to voice 3
          handle_voice_sync_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO:
          // hard-syncs frequency of voice 2 to voice 1
          handle_voice_sync_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE:
          // hard-syncs frequency of voice 3 to voice 2
          handle_voice_sync_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_ONE:
          handle_voice_test_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_TWO:
          handle_voice_test_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_THREE:
          handle_voice_test_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE:
          pw_v1 = ((word)controller_value) << 5;
          pw_v1 += pw_v1_lsb;
          handle_voice_pulse_width_change(0, pw_v1);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO:
          pw_v2 = ((word)controller_value) << 5;
          pw_v2 += pw_v2_lsb;
          handle_voice_pulse_width_change(1, pw_v2);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE:
          pw_v3 = ((word)controller_value) << 5;
          pw_v3 += pw_v3_lsb;
          handle_voice_pulse_width_change(2, pw_v3);
          break;

        // LSB messages will not trigger PW change on the SID!
        // must be followed up by a MSB message to trigger a SID update w/ both
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_ONE:
          pw_v1_lsb = controller_value & 0b00011111;
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_TWO:
          pw_v2_lsb = controller_value & 0b00011111;
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_THREE:
          pw_v3_lsb = controller_value & 0b00011111;
          break;

        case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_ONE:
          handle_voice_attack_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO:
          handle_voice_attack_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE:
          handle_voice_attack_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE:
          handle_voice_decay_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO:
          handle_voice_decay_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE:
          handle_voice_decay_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE:
          handle_voice_sustain_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO:
          handle_voice_sustain_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE:
          handle_voice_sustain_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE:
          handle_voice_release_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO:
          handle_voice_release_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE:
          handle_voice_release_change(2, controller_value >> 3);
          break;

        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_ONE:
          handle_voice_filter_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_TWO:
          handle_voice_filter_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE:
          handle_voice_filter_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE:
          handle_voice_detune_change(0, ((controller_value / 64.0) - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO:
          handle_voice_detune_change(1, ((controller_value / 64.0) - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE:
          handle_voice_detune_change(2, ((controller_value / 64.0) - 1));
          break;

        case MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_LP:
          sid_set_filter_mode(SID_FILTER_LP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_BP:
          sid_set_filter_mode(SID_FILTER_BP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_HP:
          sid_set_filter_mode(SID_FILTER_HP, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY:
          filter_frequency = (((word)controller_value) << 4) + filter_frequency_lsb;
          sid_set_filter_frequency(filter_frequency);
          break;
        // same as PW LSB message, LSB will not trigger a SID update until we receive the MSB message next.
        case MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY_LSB:
          filter_frequency_lsb = (controller_value & 0b00001111);
          break;

        case MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE:
          controller_value = constrain(controller_value, 0, 15);
          sid_set_filter_resonance(controller_value);
          break;

        case MIDI_CONTROL_CHANGE_SET_VOLUME:
          sid_set_volume(controller_value >> 3);
          break;

        case MIDI_CONTROL_CHANGE_RPN_LSB:
          rpn_value += (controller_value & 0b01111111);
          break;

        case MIDI_CONTROL_CHANGE_RPN_MSB:
          rpn_value = ((word)controller_value) << 5;
          break;

        case MIDI_CONTROL_CHANGE_DATA_ENTRY:
          data_entry = controller_value;
          if (rpn_value == MIDI_RPN_PITCH_BEND_SENSITIVITY) {
            midi_pitch_bend_max_semitones = data_entry;
            detune_max_semitones = data_entry;
          }
          break;

        case MIDI_CONTROL_CHANGE_TOGGLE_VOLUME_MODULATION_MODE:
          volume_modulation_mode_active = (controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_PULSE_WIDTH_MODULATION_MODE:
          if (controller_value == 127) {
            enable_pulse_width_modulation_mode();
          } else {
            disable_pulse_width_modulation_mode();
          }
          break;

        case MIDI_CONTROL_CHANGE_SET_GLIDE_TIME_LSB:
          glide_time_raw_lsb = controller_value;
          break;

        case MIDI_CONTROL_CHANGE_SET_GLIDE_TIME: // controller_value is 7-bit
          glide_time_raw_word = (((word)controller_value) << 7) + glide_time_raw_lsb;
          glide_time_millis = ((glide_time_raw_word / 16383.0) * (GLIDE_TIME_MAX_MILLIS - GLIDE_TIME_MIN_MILLIS)) + GLIDE_TIME_MIN_MILLIS;
          break;

        case MIDI_CONTROL_CHANGE_TOGGLE_ALL_TEST_BITS:
          sid_set_test(0, controller_value == 127);
          sid_set_test(1, controller_value == 127);
          sid_set_test(2, controller_value == 127);
          break;

        case 127:
          if (controller_value == 127) {
            handle_state_dump_request(true);
          }
          break;
        }
        break;
      case MIDI_PROGRAM_CHANGE:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();

        #if DEBUG_LOGGING
          Serial.print("\n");
          Serial.print("[");
          Serial.print(time_in_micros);
          Serial.print("] ");
          Serial.print("Received MIDI PC ");
          Serial.print(data_byte_one);
          Serial.print("\n");
        #endif

        handle_program_change(data_byte_one);
        break;

      case MIDI_PITCH_BEND:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read();
        pitchbend = data_byte_two;
        pitchbend = (pitchbend << 7);
        pitchbend |= data_byte_one;

        #if DEBUG_LOGGING
          Serial.print("\n");
          Serial.print("[");
          Serial.print(time_in_micros);
          Serial.print("] ");
          Serial.print("Received MIDI PB ");
          Serial.print(pitchbend);
          Serial.print("\n");
        #endif

        handle_pitchbend_change(pitchbend);
        break;
      case MIDI_NOTE_ON:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read(); // "velocity", which we don't use

        #if DEBUG_LOGGING
          Serial.print("\n");
          Serial.print("[");
          Serial.print(time_in_micros);
          Serial.print("] ");
          Serial.print("Received MIDI Note On ");
          Serial.print(data_byte_one);
          Serial.print("\n");
        #endif

        if (data_byte_one < 96) { // SID can't handle freqs > B7
          handle_note_on(data_byte_one);
        }
        break;
      case MIDI_NOTE_OFF:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read(); // "velocity", which we don't use

        #if DEBUG_LOGGING
          Serial.print("\n");
          Serial.print("[");
          Serial.print(time_in_micros);
          Serial.print("] ");
          Serial.print("Received MIDI Note Off ");
          Serial.print(data_byte_one);
          Serial.print("\n");
        #endif

        if (data_byte_one < 96) { // SID can't handle freqs > B7
          handle_note_off(data_byte_one);
        }
        break;
      }
    }
  }
}

void clean_slate() {
  memset(sid_state_bytes, 0, 25 * sizeof(*sid_state_bytes));
  memset(voice_detune_percents, 0, MAX_POLYPHONY*sizeof(*voice_detune_percents));
  deque_empty(notes);
  nullify_notes_playing();

  #if DEBUG_LOGGING
    unsigned int dq_bytes = sizeof(deque);
    unsigned int ht_bytes = sizeof(hash_table);
    unsigned int db_bytes = sizeof(notes->ht->array) * sizeof(maybe_hash_table_element);

    Serial.print(F("allocated hash_deque bytes: "));
    Serial.print(dq_bytes);
    Serial.print(F(" + "));
    Serial.print(ht_bytes);
    Serial.print(F(" + "));
    Serial.print(db_bytes);
    Serial.print(F(" = "));
    Serial.print(dq_bytes + ht_bytes + db_bytes);
    Serial.print(F(" bytes\n"));
  #endif

  polyphony = 3;
  legato_mode = false;
  glide_time_millis = DEFAULT_GLIDE_TIME_MILLIS;
  glide_time_raw_word = 0;
  glide_time_raw_lsb = 0;
  glide_start_time_micros = 0;
  glide_to = 0;
  glide_from = 0;
  midi_pitch_bend_max_semitones = 5;
  current_pitchbend_amount = 0.0;
  detune_max_semitones = 5;
  pw_v1     = 0;
  pw_v1_lsb = 0;
  pw_v2     = 0;
  pw_v2_lsb = 0;
  pw_v3     = 0;
  pw_v3_lsb = 0;
  filter_frequency = 0;
  filter_frequency_lsb = 0;
  rpn_value = 0;
  data_entry = 0;
  volume_modulation_mode_active = false;
  pulse_width_modulation_mode_active = false;
  last_update = 0;

  sid_zero_all_registers();
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    sid_set_pulse_width(i, DEFAULT_PULSE_WIDTH);
    sid_set_waveform(i, DEFAULT_WAVEFORM);
    sid_set_attack(i, DEFAULT_ATTACK);
    sid_set_decay(i, DEFAULT_DECAY);
    sid_set_sustain(i, DEFAULT_SUSTAIN);
    sid_set_release(i, DEFAULT_RELEASE);
  }
  sid_set_filter_frequency(DEFAULT_FILTER_FREQUENCY);
  sid_set_filter_resonance(DEFAULT_FILTER_RESONANCE);
  sid_set_volume(DEFAULT_VOLUME);
}

void setup() {
  setup_stdin_stdout();
  notes->stream = stdout;

  DDRF |= 0B01110011; // initialize 5 PORTF pins as output (connected to A0-A4)
  DDRB = 0B11111111; // initialize 8 PORTB pins as output (connected to D0-D7)
  // technically SID allows us to read from its last 4 registers, but we don't
  // need to, so we just always keep SID's R/W pin low (signifying "write"),
  // which seems to work ok

  pinMode(ARDUINO_SID_MASTER_CLOCK_PIN, OUTPUT);
  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  start_clock();
  cs_high();

  clean_slate();
}

void loop () {
  time_in_micros = micros();
  time_in_seconds = (unsigned long) (time_in_micros / 1000000.0);

  // SID has a bug where its oscillators sometimes "leak" the sound of previous
  // notes. To work around this, we have to set each oscillator's frequency to 0
  // only when we are certain it's past its ADSR time.
  for (int i = 0; i < 3; i++) {
    if (oscillator_notes[i].off_time > 0 && (time_in_micros > (oscillator_notes[i].off_time + get_release_seconds(i) * 1000000.0))) {
      // we're past the release phase, so the voice can't be making any noise, so we must "fully" silence it
      sid_set_voice_frequency(i, 0);
      oscillator_notes[i].on_time = 0;
      oscillator_notes[i].off_time = 0;

      #if DEBUG_LOGGING
        Serial.print("leak detector got one: ");
        byte _n = oscillator_notes[i].number;
        Serial.print(_n);
        Serial.print("\n");
      #endif

      deque_remove_by_key(notes, oscillator_notes[i].number);
      oscillator_notes[i].number = 0;
    }
  }

  // manually update oscillator frequencies to account for glide times
  if (legato_mode &&
      glide_time_millis > 0 &&
      (oscillator_notes[0].number != 0 || oscillator_notes[1].number != 0 || oscillator_notes[2].number != 0 ) &&
      ((time_in_micros - (glide_time_millis * 1000.0)) <= glide_start_time_micros) &&
      ((time_in_micros - last_glide_update_micros) > update_every_micros)) {

    double glide_duration_so_far_millis = (time_in_micros - glide_start_time_micros) / 1000.0;
    double glide_percentage_progress = glide_duration_so_far_millis / glide_time_millis;
    double glide_frequency_distance = 0.0;
    double glide_hertz_to_add = 0.0;
    double freq_after_pb_and_detune = 0.0;

    for (int i = 0; i < MAX_POLYPHONY; i++) {
      if (oscillator_notes[i].number != 0) {
        freq_after_pb_and_detune = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
        freq_after_pb_and_detune = note_number_to_frequency(glide_from) * pow(2, freq_after_pb_and_detune / 12.0);

        glide_frequency_distance = note_number_to_frequency(glide_to) - note_number_to_frequency(glide_from);
        glide_hertz_to_add = glide_frequency_distance * glide_percentage_progress;

        sid_set_voice_frequency(i, (word)(freq_after_pb_and_detune + glide_hertz_to_add));
      }
    }

    last_glide_update_micros = micros();
  }

  // if any notes are playing in `volume_modulation_mode`, we have to implement
  // ADSR stuff on our own.
  if (
    volume_modulation_mode_active &&
    (oscillator_notes[0].number != 0 || oscillator_notes[1].number != 0 || oscillator_notes[2].number != 0 ) &&
    ((time_in_micros - last_update) > update_every_micros)) {
    double volume = 0;
    int oscillator_notes_count = 0;
    for (int i = 0; i < 3; i++) {
      oscillator_notes[i].number != 0 && oscillator_notes_count++;
    }

    for (int i = 0; i < oscillator_notes_count; i++) {
      int note = oscillator_notes[i].number;

      if (note != 0) {
        double note_frequency = note_number_to_frequency(note);
        double temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
        note_frequency = note_frequency * pow(2, temp_double / 12.0);

        double yt = sine_waveform(note_frequency, time_in_seconds, 0.5, 0);
        yt *= linear_envelope(
          get_attack_seconds(i),
          get_decay_seconds(i),
          get_sustain_percent(i),
          get_release_seconds(i),
          (time_in_micros - oscillator_notes[i].on_time) / 1000000.0,
          -1
        );

        volume += (((yt + 0.5) * 15) / oscillator_notes_count);
        // `+ 0.5` // center around 0
        // `* 15`  // expand to [0..15]
        // `/ 3`   // make room for potentially summing three voices together
      }
    }

    sid_set_volume((byte)volume);
    last_update = micros();
  }

  // same with `pulse_width_modulation_mode`
  if (
    pulse_width_modulation_mode_active &&
    (oscillator_notes[0].number != 0 || oscillator_notes[1].number != 0 || oscillator_notes[2].number != 0 ) &&
    ((time_in_micros - last_update) > update_every_micros)) {
    int oscillator_notes_count = 0;
    for (int i = 0; i < 3; i++) {
      if (oscillator_notes[i].number != 0) {
        oscillator_notes_count++;
      }
    }

    for (int i = 0; i < oscillator_notes_count; i++) {
      int note = oscillator_notes[i].number;
      double note_frequency = note_number_to_frequency(note);

      double temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      note_frequency = note_frequency * pow(2, temp_double / 12.0);

      if (note != 0) {
        double yt = sine_waveform(note_frequency, time_in_seconds, 0.5, 0);
        yt *= linear_envelope(
          get_attack_seconds(i),
          get_decay_seconds(i),
          get_sustain_percent(i),
          get_release_seconds(i),
          (time_in_micros - oscillator_notes[i].on_time) / 1000000.0,
          -1
        );

        sid_set_pulse_width(i, 4095.0 * yt);
      }
    }

    last_update = micros();
  }

  USBMIDI.poll();

  handle_midi_input(&USBMIDI);
  handle_midi_input(&Serial1);
}
