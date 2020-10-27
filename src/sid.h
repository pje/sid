#ifndef SRC_SID_H
#define SRC_SID_H

#include <stdbool.h>
#include "util.h"

// will be defined in SID.ino
extern void clock_high();
extern void clock_low();
extern void cs_high();
extern void cs_low();

// hack to get this running in test environments. PORT* are defined in avr/*.h
#ifndef PORTB
  byte PORTB = 0B00000000;
#endif /* PORTB */

#ifndef PORTF
  byte PORTF = 0B00000000;
#endif /* PORTF */

#ifndef cli
  void cli() {};
#endif

#ifndef sei
  void sei() {};
#endif

const byte SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO   = 0;
const byte SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI   = 1;
const byte SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO = 2;
const byte SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI = 3;
const byte SID_REGISTER_OFFSET_VOICE_CONTROL        = 4;
const byte SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD    = 5;
const byte SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR    = 6;

const byte SID_REGISTER_ADDRESS_FILTER_FREQUENCY_LO = 21;
const byte SID_REGISTER_ADDRESS_FILTER_FREQUENCY_HI = 22;
const byte SID_REGISTER_ADDRESS_FILTER_RESONANCE    = 23;
const byte SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME  = 24;

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

// SID expects a 1Mhz clock signal on which to calculate oscillator frequencies
const double CLOCK_SIGNAL_FACTOR = 0.059604644775390625;

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

// since we have to set all the bits in a register byte at once,
// we must maintain a copy of the register's state so we don't clobber bits
// (SID actually has 28 registers but we don't use the last 4. hence 25)
byte sid_state_bytes[25] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

void sid_transfer(byte address, byte data);
void sid_zero_all_registers();
void sid_set_volume(byte level);
void sid_set_waveform(byte voice, byte waveform_mask, bool on);
void sid_toggle_waveform(byte voice, byte waveform_mask, bool on);
void sid_set_ring_mod(byte voice, bool on);
void sid_set_test(byte voice, bool on);
void sid_set_sync(byte voice, bool on);
void sid_set_attack(byte voice, byte attack);
void sid_set_decay(byte voice, byte decay);
void sid_set_sustain(byte voice, byte sustain);
void sid_set_release(byte voice, byte release);
void sid_set_pulse_width(byte voice, word hertz); // 12-bit value
void sid_set_filter_frequency(word hertz); // 11-bit value
void sid_set_filter_resonance(byte amount);
void sid_set_filter(byte voice, bool on);
void sid_set_filter_mode(byte mode, bool on);
void sid_set_voice_frequency(byte voice, double hertz);
void sid_set_gate(byte voice, bool state);
// NB: getters return our current tally of what we've sent to the SID. We can't actually read register values from SID.
word get_voice_frequency_register_value(byte voice);
float get_voice_frequency(byte voice);
word get_voice_pulse_width(byte voice);
byte get_voice_waveform(byte voice);
bool get_voice_test_bit(byte voice);
bool get_voice_ring_mod(byte voice);
bool get_voice_sync(byte voice);
bool get_voice_gate(byte voice);
float get_attack_seconds(byte voice);
float get_decay_seconds(byte voice);
float get_sustain_percent(byte voice);
float get_release_seconds(byte voice);
word get_filter_frequency();
byte get_filter_resonance();
byte get_volume();
bool get_filter_enabled_for_voice(byte voice);

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

  cli(); // same as `noInterrupts()`

  clock_high();
  clock_low();

  PORTF = data_for_port_f;
  PORTB = data;

  cs_low();
  clock_high();

  clock_low();
  cs_high();

  sei(); // same as `interrupts()`

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

void sid_set_waveform(byte voice, byte waveform_mask, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address] & 0B00001111;

  if (on) {
    data |= waveform_mask;
  } else {
    data &= ~waveform_mask;
  }

  sid_transfer(address, data);
}

void sid_toggle_waveform(byte voice, byte waveform_mask, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address];

  if (on) {
    data |= waveform_mask;
  } else {
    data &= ~waveform_mask;
  }

  sid_transfer(address, data);
}

 // ring mod repurposes the output of the triangle oscillator
void sid_set_ring_mod(byte voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address];

  if (on) {
    data |= SID_RING;
  } else {
    data &= ~SID_RING;
  }

  sid_transfer(address, data);
}

void sid_set_test(byte voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address];

  if (on) {
    data |= SID_TEST;
  } else {
    data &= ~SID_TEST;
  }

  sid_transfer(address, data);
}

void sid_set_sync(byte voice, bool on) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address];

  if (on) {
    data |= SID_SYNC;
  } else {
    data &= ~SID_SYNC;
  }

  sid_transfer(address, data);
}

void sid_set_attack(byte voice, byte attack) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & 0B00001111;
  data |= (attack << 4);
  sid_transfer(address, data);
}

void sid_set_decay(byte voice, byte decay) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & 0B11110000;
  data |= (decay & 0B00001111);
  sid_transfer(address, data);
}

void sid_set_sustain(byte voice, byte sustain) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & 0B00001111;
  data |= (sustain << 4);
  sid_transfer(address, data);
}

void sid_set_release(byte voice, byte release) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & 0B11110000;
  data |= (release & 0B00001111);
  sid_transfer(address, data);
}

void sid_set_pulse_width(byte voice, word hertz) { // 12-bit value
  byte hi = highByte(hertz) & 0B00001111;
  byte lo = lowByte(hertz);
  byte address_hi = (voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI;
  byte address_lo = (voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO;
  sid_transfer(address_hi, hi);
  sid_transfer(address_lo, lo);
}

void sid_set_filter_frequency(word hertz) { // 11-bit value
  byte hi = highByte(hertz << 5);
  byte lo = lowByte(hertz) & 0B00000111;
  sid_transfer(SID_REGISTER_ADDRESS_FILTER_FREQUENCY_HI, hi);
  sid_transfer(SID_REGISTER_ADDRESS_FILTER_FREQUENCY_LO, lo);
}

void sid_set_filter_resonance(byte amount) {
  byte address = SID_REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data = (sid_state_bytes[address] & 0B00001111) | (amount << 4);
  sid_transfer(address, data);
}

void sid_set_filter(byte voice, bool on) {
  voice = constrain(voice, 0, 3); // allow `3` to mean "ext filt on/off"

  byte address = SID_REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data = sid_state_bytes[address];
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
    data |= voice_filter_mask;
  } else {
    data &= ~voice_filter_mask;
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

void sid_set_voice_frequency(byte voice, double hertz) {
  word frequency = round(hertz / CLOCK_SIGNAL_FACTOR);
  byte hiFrequency = highByte(frequency);
  byte loFrequency = lowByte(frequency);

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

void sid_set_gate(byte voice, bool state) {
  byte address = (voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL;
  byte data = sid_state_bytes[address];

  if (state) {
    data |= SID_GATE;
  } else {
    data &= ~SID_GATE;
  }

  sid_transfer(address, data);
}

word get_voice_frequency_register_value(byte voice) {
  word value = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI];
  value <<= 8;
  value += sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO];
  return value;
}

float get_voice_frequency(byte voice) {
  word frequency = get_voice_frequency_register_value(voice);
  double hertz = frequency * CLOCK_SIGNAL_FACTOR;
  return(hertz);
}

word get_voice_pulse_width(byte voice) {
  word pulse_width = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_HI];
  pulse_width <<= 8;
  pulse_width += sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_PULSE_WIDTH_LO];
  return(pulse_width);
}

byte get_voice_waveform(byte voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & 0B11110000) >> 4);
}

bool get_voice_test_bit(byte voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_TEST) != 0);
}

bool get_voice_ring_mod(byte voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_RING) != 0);
}

bool get_voice_sync(byte voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_SYNC) != 0);
}

bool get_voice_gate(byte voice) {
  byte control_register = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_CONTROL];
  return((control_register & SID_GATE) != 0);
}

float get_attack_seconds(byte voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_attack_values_to_seconds[value]);
}

float get_decay_seconds(byte voice) {
  byte value = lowNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_decay_and_release_values_to_seconds[value]);
}

// returns float [0..1]
float get_sustain_percent(byte voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_ENVELOPE_SR]);
  return((float) value / 15.0);
}

float get_release_seconds(byte voice) {
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
  byte resonance = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_RESONANCE] & 0B11110000;
  resonance >>= 4;
  return(resonance);
}

byte get_volume() {
  return(sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME] & 0B00001111);
}

bool get_filter_enabled_for_voice(byte voice) {
  byte bits = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_RESONANCE] & 0B00000111;
  bits >>= voice;
  bits <<= (8 - (voice + 1));
  return(bits != 0);
}

#endif /* SRC_SID_H */
