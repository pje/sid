#include "Arduino.h"
#include <avr/io.h>
#include <math.h>
#include <usbmidi.h>
#include "util.h"

typedef unsigned char byte;
typedef unsigned int word;

const bool DEBUG_SID_TRANSFER = false; // setting to true will print every SID data transfer
const bool DEBUG_GLIDE_UPDATES = false;

const int ARDUINO_SID_CHIP_SELECT_PIN = 13; // D13
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5; // D5

const double CLOCK_SIGNAL_FACTOR = 0.0596;

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

const double SID_MIN_OSCILLATOR_HERTZ = 16.35;
const double SID_MAX_OSCILLATOR_HERTZ = 3951.06;

const char *sid_register_short_names[25] = {
  "V1 FREQ LO ",
  "V1 FREQ HI ",
  "V1 PW LO   ",
  "V1 PW HI   ",
  "V1 CONTROL ",
  "V1 ATK/DCY ",
  "V1 STN/RLS ",
  "V2 FREQ LO ",
  "V2 FREQ HI ",
  "V2 PW LO   ",
  "V2 PW HI   ",
  "V2 CONTROL ",
  "V2 ATK/DCY ",
  "V2 STN/RLS ",
  "V3 FREQ LO ",
  "V3 FREQ HI ",
  "V3 PW LO   ",
  "V3 PW HI   ",
  "V3 CONTROL ",
  "V3 ATK/DCY ",
  "V3 STN/RLS ",
  "FG FC LO   ",
  "FG FC HI   ",
  "FG RES/FILT",
  "FG MODE/VOL"
};

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
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME_LSB                = 124; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME                    = 125; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_TOGGLE_ALL_TEST_BITS              = 126; // 1-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_VOLUME_MODULATION_MODE     = 81; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_PULSE_WIDTH_MODULATION_MODE= 83; // 1-bit value

const byte MIDI_CONTROL_CHANGE_RPN_MSB                           = 101;
const byte MIDI_CONTROL_CHANGE_RPN_LSB                           = 100;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY                        = 6;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY_FINE                   = 38;
const word MIDI_RPN_PITCH_BEND_SENSITIVITY                       = 0;
const word MIDI_RPN_MASTER_FINE_TUNING                           = 1;
const word MIDI_RPN_MASTER_COARSE_TUNING                         = 2;
const word MIDI_RPN_NULL                                         = 16383;

const byte MIDI_CHANNEL = 0; // "channel 1" (zero-indexed)
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_POLYPHONIC        = 0;
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC        = 1;
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC_LEGATO = 2;
const byte MIDI_PROGRAM_CHANGE_HARDWARE_RESET                    = 127;

const byte MAX_POLYPHONY = 3;
const double DEFAULT_PITCH_BEND_SEMITONES = 5;
const double DEFAULT_GLIDE_TIME_MILLIS = 100.0;
const double GLIDE_TIME_MIN_MILLIS = 1;
const double GLIDE_TIME_MAX_MILLIS = 10000;

byte polyphony = 3;
bool legato_mode = false;
double glide_time_millis = DEFAULT_GLIDE_TIME_MILLIS;
word glide_time_raw_word;
byte glide_time_raw_lsb;
unsigned long glide_start_time_micros = 0;
byte glide_to = 0;
byte glide_from = 0;

struct note {
  byte number;
  unsigned long on_time;
  unsigned long off_time;
};

note notes_playing[MAX_POLYPHONY]  = { { .number = 0, .on_time = 0, .off_time = 0 } };

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
const double DEFAULT_FILTER_FREQUENCY = 1000.0;
const byte DEFAULT_FILTER_RESONANCE = 0;
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
const double PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY = 65535;

unsigned long last_update = 0;
unsigned long last_glide_update_micros = 0;
const double update_every_micros = (100.0 / 4.41);

unsigned long time_in_micros = 0;
double time_in_seconds = 0;

const double sid_attack_values_to_seconds[16] = {
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

const double sid_decay_and_release_values_to_seconds[16] = {
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

// there's apparently not widespread agreement on which frequency midi notes
// represent. What we have below is "scientific pitch notation". Ableton, maxmsp
// and garageband use C3, which is shifted an octave lower.
// https://en.wikipedia.org/wiki/Scientific_pitch_notation#See_also
const double MIDI_NOTES_TO_FREQUENCIES[96] = {
  16.351597831287414, // C0
  17.323914436054505, // C#0
  18.354047994837977, // D0
  19.445436482630058, // D#0
  20.601722307054366, // E0
  21.826764464562746, // F0
  23.12465141947715,  // F#0
  24.499714748859326, // G0
  25.956543598746574, // G#0
  27.5,               // A0
  29.13523509488062,  // A#0
  30.86770632850775,  // B0
  32.70319566257483,  // C1
  34.64782887210901,  // C#1
  36.70809598967594,  // D1
  38.890872965260115, // D#1
  41.20344461410875,  // E1
  43.653528929125486, // F1
  46.2493028389543,   // F#1
  48.999429497718666, // G1
  51.91308719749314,  // G#1
  55.0,               // A1
  58.27047018976124,  // A#1
  61.7354126570155,   // B1
  65.40639132514966,  // C2
  69.29565774421802,  // C#2
  73.41619197935188,  // D2
  77.78174593052023,  // D#2
  82.4068892282175,   // E2
  87.30705785825097,  // F2
  92.4986056779086,   // F#2
  97.99885899543733,  // G2
  103.82617439498628, // G#2
  110.0,              // A2
  116.54094037952248, // A#2
  123.47082531403103, // B2
  130.8127826502993,  // C3
  138.59131548843604, // C#3
  146.8323839587038,  // D3
  155.56349186104046, // D#3
  164.81377845643496, // E3
  174.61411571650194, // F3
  184.9972113558172,  // F#3
  195.99771799087463, // G3
  207.65234878997256, // G#3
  220.0,              // A3
  233.08188075904496, // A#3
  246.94165062806206, // B3
  261.6255653005986,  // C4
  277.1826309768721,  // C#4
  293.6647679174076,  // D4
  311.1269837220809,  // D#4
  329.6275569128699,  // E4
  349.2282314330039,  // F4
  369.9944227116344,  // F#4
  391.99543598174927, // G4
  415.3046975799451,  // G#4
  440.0,              // A4
  466.1637615180899,  // A#4
  493.8833012561241,  // B4
  523.2511306011972,  // C5
  554.3652619537442,  // C#5
  587.3295358348151,  // D5
  622.2539674441618,  // D#5
  659.2551138257398,  // E5
  698.4564628660078,  // F5
  739.9888454232688,  // F#5
  783.9908719634985,  // G5
  830.6093951598903,  // G#5
  880.0,              // A5
  932.3275230361799,  // A#5
  987.7666025122483,  // B5
  1046.5022612023945, // C5
  1108.7305239074883, // C#6
  1174.6590716696303, // D6
  1244.5079348883237, // D#6
  1318.5102276514797, // E6
  1396.9129257320155, // F6
  1479.9776908465376, // F#6
  1567.981743926997,  // G6
  1661.2187903197805, // G#6
  1760.0,             // A6
  1864.6550460723597, // A#6
  1975.533205024496,  // B6
  2093.004522404789,  // C6
  2217.4610478149766, // C#7
  2349.31814333926,   // D7
  2489.0158697766474, // D#7
  2637.02045530296,   // E7
  2793.825851464031,  // F7
  2959.955381693075,  // F#7
  3135.9634878539946, // G7
  3322.437580639561,  // G#7
  3520.0,             // A7
  3729.3100921447194, // A#7
  3951.066410048992   // B7
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

double get_voice_frequency(unsigned int voice) {
  word frequency = sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI];
  frequency <<= 8;
  frequency += sid_state_bytes[(voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO];
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
  cs_low();
  clock_high();

  PORTF = data_for_port_f;
  PORTB = data;

  clock_low();
  cs_high();

  interrupts();

  sid_state_bytes[address] = data;

  if (DEBUG_SID_TRANSFER) {
    Serial.print("sid_transfer(");
    Serial.print(address, BIN);
    Serial.print(", ");
    Serial.print(data, BIN);
    Serial.print(")\n");
  }
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
  byte loAddress = (voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_LO;
  byte hiAddress = (voice * 7) + SID_REGISTER_OFFSET_VOICE_FREQUENCY_HI;
  sid_transfer(loAddress, loFrequency);
  sid_transfer(hiAddress, hiFrequency);
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
  pinMode(ARDUINO_SID_MASTER_CLOCK_PIN, OUTPUT);
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
    notes_playing[i] = { .number = 0, .on_time = 0, .off_time = 0 };
  }
}

void handle_message_voice_attack_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_attack(i, envelope_value);
    }
  } else {
    sid_set_attack(voice, envelope_value);
  }
}

void handle_message_voice_decay_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_decay(i, envelope_value);
    }
  } else {
    sid_set_decay(voice, envelope_value);
  }
}

void handle_message_voice_sustain_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_sustain(i, envelope_value);
    }
  } else {
    sid_set_sustain(voice, envelope_value);
  }
}

void handle_message_voice_release_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_release(i, envelope_value);
    }
  } else {
    sid_set_release(voice, envelope_value);
  }
}

void handle_message_voice_waveform_change(byte voice, byte waveform, bool on) {
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

void handle_message_voice_filter_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < 3; i++) {
      sid_set_filter(i, on);
    }
  } else {
    sid_set_filter(voice, on);
  }
}

void handle_message_voice_pulse_width_change(byte voice, word frequency) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_pulse_width(i, frequency);
    }
  } else {
    sid_set_pulse_width(voice, frequency);
  }
}

void handle_message_voice_ring_mod_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_ring_mod(i, on);
    }
  } else {
    sid_set_ring_mod(voice, on);
  }
}

void handle_message_voice_sync_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_sync(i, on);
    }
  } else {
    sid_set_sync(voice, on);
  }
}

void handle_message_voice_test_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (int i = 0; i < polyphony; i++) {
      sid_set_test(i, on);
    }
  } else {
    sid_set_test(voice, on);
  }
}

void handle_message_voice_detune_change(byte voice, double detune_factor) {
  voice_detune_percents[voice] = detune_factor;
  double semitone_change = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[voice] * detune_max_semitones);
  double frequency = MIDI_NOTES_TO_FREQUENCIES[notes_playing[voice].number] * pow(2, semitone_change / 12.0);
  sid_set_voice_frequency(voice, frequency);
}

// set the oscillator frequency and gate it!
// - takes stateful stuff into account, like pitch bend and detune
// - will first de-gate the oscillator iff it's not already de-gated for some reason
// - handles global modulation modes (if we're in volume mod mode, we don't actually interact with the voice.)
void play_note_for_voice(byte note_number, unsigned int voice) {
  double hertz = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[voice] * detune_max_semitones);
  hertz = MIDI_NOTES_TO_FREQUENCIES[note_number] * pow(2, hertz / 12.0);

  if (!volume_modulation_mode_active) {
    if (get_voice_gate(voice)) {
      if (legato_mode) {
        glide_start_time_micros = micros();
        glide_to = note_number;
        glide_from = notes_playing[voice].number;
      } else {
        sid_set_gate(voice, false);
      }
    }
    if (pulse_width_modulation_mode_active) {
      sid_set_voice_frequency(voice, PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY);
    } else {
      if (!legato_mode || notes_playing[voice].number == 0) {
        sid_set_voice_frequency(voice, hertz);
      }
    }
    if (!get_voice_gate(voice)) {
      sid_set_gate(voice, true);
      notes_playing[voice].on_time = micros();
    }
  }

  notes_playing[voice].number = note_number;
  notes_playing[voice].off_time = 0;
}

void handle_message_note_on(byte note_number) {
  if (polyphony == 1) {
    for (int i = 0; i < MAX_POLYPHONY; i++ ) {
      if (get_voice_waveform(i) != 0) {
        play_note_for_voice(note_number, i);
      }
    }
    return;
  }

  // we're poly, so every voice has its own frequency.
  // if there's a free voice, we can just use that.
  for (int i = 0; i < polyphony; i++) {
    if (notes_playing[i].number == 0) {
      play_note_for_voice(note_number, i);
      return;
    }
  }

  // there are no free voices, so find the oldest one and replace it.
  unsigned int oldest_voice = 0;
  for (int i = 1; i < polyphony; i++) {
    if (notes_playing[i].number != 0 && notes_playing[i].on_time < notes_playing[oldest_voice].on_time) {
      oldest_voice = i;
    }
  }
  play_note_for_voice(note_number, oldest_voice);
}

void handle_message_note_off(byte note_number) {
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    if (notes_playing[i].number == note_number) {
      if (!pulse_width_modulation_mode_active) {
        sid_set_gate(i, false);
      }
      notes_playing[i] = { .number = 0, .on_time = 0, .off_time = micros() };
    }
  }

  if (note_number == glide_to) {
    glide_from = 0;
    glide_to = 0;
  }
}

void handle_message_pitchbend_change(word pitchbend) {
  double temp_double = 0.0;
  current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the "neutral" pitchbend value (half of 2**14)
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    if (notes_playing[i].number != 0) {
      temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      temp_double = MIDI_NOTES_TO_FREQUENCIES[notes_playing[i].number] * pow(2, temp_double / 12.0);
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

void handle_message_program_change(byte program_number) {
  switch (program_number) {
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_POLYPHONIC:
    polyphony = 3;
    sid_set_gate(0, false);
    sid_set_gate(1, false);
    sid_set_gate(2, false);
    duplicate_voice(0, 1);
    duplicate_voice(0, 2);
    for (int i = 0; i < 3; i++) {
      notes_playing[i] = { .number = 0, .on_time = 0, .off_time = 0 };
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
    if (' ' == str[i]) str[i] = padding;
  }
}

void handle_state_dump_request(bool human) {
  // for testing purposes, print the state of our sid representation, which
  // should (hopefully) mirror what the SID's registers are right now

  if (human) {
    Serial.print("V#  WAVE     FREQ    A      D      S      R      PW   TEST RING SYNC GATE FILT\n");
    //           "V1  Triangle 1234.56 12.345 12.345 12.345 12.345 2048 1    1    1    1    1   \n"                                                            "
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
      // dtostrf(f, 7, 2, float_string);
      // sprintf(float_string, "%7s", float_string);
      // Serial.print(" ");
      // Serial.print(float_string);

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

    Serial.print("Notes: {");
    Serial.print(notes_playing[0].number);
    Serial.print(", ");
    Serial.print(notes_playing[1].number);
    Serial.print(", ");
    Serial.print(notes_playing[2].number);
    Serial.print("}\n");

    Serial.print("note_on_times: {");
    Serial.print(notes_playing[0].on_time);
    Serial.print(", ");
    Serial.print(notes_playing[1].on_time);
    Serial.print(", ");
    Serial.print(notes_playing[2].on_time);
    Serial.print("}\n");

    Serial.print("note_off_times: {");
    Serial.print(notes_playing[0].off_time);
    Serial.print(", ");
    Serial.print(notes_playing[1].off_time);
    Serial.print(", ");
    Serial.print(notes_playing[2].off_time);
    Serial.print("}\n");

  } else {
    for (int i = 0; i < 25; i++) {
      static char str[9];
      str[0] = '\0';

      for (int j = 128; j > 0; j >>= 1) {
        strcat(str, ((sid_state_bytes[i] & j) == j) ? "1" : "0");
      }

      Serial.print(str);
      Serial.print("  // ");
      Serial.print(sid_register_short_names[i]);
      Serial.print("\n");
    }
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

        switch (controller_number) {
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_SQUARE:
          handle_message_voice_waveform_change(0, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_SQUARE:
          handle_message_voice_waveform_change(1, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_SQUARE:
          handle_message_voice_waveform_change(2, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_TRIANGLE:
          handle_message_voice_waveform_change(0, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_TRIANGLE:
          handle_message_voice_waveform_change(1, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_TRIANGLE:
          handle_message_voice_waveform_change(2, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_RAMP:
          handle_message_voice_waveform_change(0, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_RAMP:
          handle_message_voice_waveform_change(1, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_RAMP:
          handle_message_voice_waveform_change(2, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_NOISE:
          handle_message_voice_waveform_change(0, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_NOISE:
          handle_message_voice_waveform_change(1, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_NOISE:
          handle_message_voice_waveform_change(2, SID_NOISE, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE:
          // replaces the triangle output of voice 1 with a ring modulated combination of voice 1 by voice 3
          handle_message_voice_ring_mod_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO:
          // replaces the triangle output of voice 2 with a ring modulated combination of voice 2 by voice 1
          handle_message_voice_ring_mod_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE:
          // replaces the triangle output of voice 3 with a ring modulated combination of voice 3 by voice 2
          handle_message_voice_ring_mod_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE:
          // hard-syncs frequency of voice 1 to voice 3
          handle_message_voice_sync_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO:
          // hard-syncs frequency of voice 2 to voice 1
          handle_message_voice_sync_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE:
          // hard-syncs frequency of voice 3 to voice 2
          handle_message_voice_sync_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_ONE:
          handle_message_voice_test_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_TWO:
          handle_message_voice_test_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_TEST_VOICE_THREE:
          handle_message_voice_test_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE:
          pw_v1 = ((word)controller_value) << 5;
          pw_v1 += pw_v1_lsb;
          handle_message_voice_pulse_width_change(0, pw_v1);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO:
          pw_v2 = ((word)controller_value) << 5;
          pw_v2 += pw_v2_lsb;
          handle_message_voice_pulse_width_change(1, pw_v2);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE:
          pw_v3 = ((word)controller_value) << 5;
          pw_v3 += pw_v3_lsb;
          handle_message_voice_pulse_width_change(2, pw_v3);
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
          handle_message_voice_attack_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO:
          handle_message_voice_attack_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE:
          handle_message_voice_attack_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE:
          handle_message_voice_decay_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO:
          handle_message_voice_decay_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE:
          handle_message_voice_decay_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE:
          handle_message_voice_sustain_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO:
          handle_message_voice_sustain_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE:
          handle_message_voice_sustain_change(2, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE:
          handle_message_voice_release_change(0, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO:
          handle_message_voice_release_change(1, controller_value >> 3);
          break;
        case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE:
          handle_message_voice_release_change(2, controller_value >> 3);
          break;

        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_ONE:
          handle_message_voice_filter_change(0, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_TWO:
          handle_message_voice_filter_change(1, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE:
          handle_message_voice_filter_change(2, controller_value == 127);
          break;

        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE:
          handle_message_voice_detune_change(0, ((controller_value / 64.0) - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO:
          handle_message_voice_detune_change(1, ((controller_value / 64.0) - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE:
          handle_message_voice_detune_change(2, ((controller_value / 64.0) - 1));
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
          sid_set_filter_resonance(constrain(controller_value, 0, 15));
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
        handle_message_program_change(data_byte_one);
        break;

      case MIDI_PITCH_BEND:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read();
        pitchbend = data_byte_two;
        pitchbend = (pitchbend << 7);
        pitchbend |= data_byte_one;
        handle_message_pitchbend_change(pitchbend);
        break;
      case MIDI_NOTE_ON:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read(); // "velocity", which we don't use
        if (data_byte_one < 96) { // SID can't handle freqs > B7
          handle_message_note_on(data_byte_one);
        }
        break;
      case MIDI_NOTE_OFF:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read(); // "velocity", which we don't use
        handle_message_note_off(data_byte_one);
        break;
      }
    }
  }
}

void clean_slate() {
  memset(sid_state_bytes, 0, 25*sizeof(*sid_state_bytes));
  memset(voice_detune_percents, 0, MAX_POLYPHONY*sizeof(*voice_detune_percents));
  nullify_notes_playing();

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
  DDRF = 0B01110011; // initialize 5 PORTF pins as output (connected to A0-A4)
  DDRB = 0B11111111; // initialize 8 PORTB pins as output (connected to D0-D7)
  // technically SID allows us to read from its last 4 registers, but we don't
  // need to, so we just always keep SID's R/W pin low (signifying "write"),
  // which seems to work ok

  start_clock();

  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  Serial.begin(31250);

  clean_slate();
}

void loop () {
  time_in_micros = micros();
  time_in_seconds = time_in_micros / 1000000.0;

  // SID has a bug where its oscillators sometimes "leak" the sound of previous
  // notes. To work around this, we have to set each oscillator's frequency to 0
  // only when we are certain it's past its ADSR time.
  for (int i = 0; i < 3; i++) {
    if (notes_playing[i].number == 0 && notes_playing[i].off_time > 0 && (time_in_micros > (notes_playing[i].off_time + get_release_seconds(i) * 1000000.0))) {
      // we're past the release phase, so the voice can't be making any noise, so we must "fully" silence it
      sid_set_voice_frequency(i, 0);
      notes_playing[i].off_time = 0;
    }
  }

  // manually update oscillator frequencies to account for glide times
  if (legato_mode &&
      glide_time_millis > 0 &&
      (notes_playing[0].number != 0 || notes_playing[1].number != 0 || notes_playing[2].number != 0 ) &&
      ((time_in_micros - (glide_time_millis * 1000.0)) <= glide_start_time_micros) &&
      ((time_in_micros - last_glide_update_micros) > update_every_micros)) {

    double glide_duration_so_far_millis = (time_in_micros - glide_start_time_micros) / 1000.0;
    double glide_percentage_progress = glide_duration_so_far_millis / glide_time_millis;
    double glide_frequency_distance = 0.0;
    double glide_hertz_to_add = 0.0;
    double freq_after_pb_and_detune = 0.0;

    for (int i = 0; i < MAX_POLYPHONY; i++) {
      if (notes_playing[i].number != 0) {
        freq_after_pb_and_detune = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
        freq_after_pb_and_detune = MIDI_NOTES_TO_FREQUENCIES[glide_from] * pow(2, freq_after_pb_and_detune / 12.0);

        glide_frequency_distance = MIDI_NOTES_TO_FREQUENCIES[glide_to] - MIDI_NOTES_TO_FREQUENCIES[glide_from];
        glide_hertz_to_add = glide_frequency_distance * glide_percentage_progress;

        sid_set_voice_frequency(i, (word)(freq_after_pb_and_detune + glide_hertz_to_add));

        if (DEBUG_GLIDE_UPDATES && i == 0) {
          Serial.print(time_in_micros);
          Serial.print("  from: ");
          Serial.print(glide_from);
          Serial.print("  to: ");
          Serial.print(glide_to);
          Serial.print("  freq: ");
          Serial.print(get_voice_frequency(i));
          Serial.print("\n");
          Serial.print("  glide_time_millis: ");
          Serial.print(glide_time_millis);
          Serial.print("\n");
          Serial.print("  glide_from_hz: ");
          Serial.print(MIDI_NOTES_TO_FREQUENCIES[glide_from]);
          Serial.print("\n");
          Serial.print("  glide_to_hz: ");
          Serial.print(MIDI_NOTES_TO_FREQUENCIES[glide_to]);
          Serial.print("\n");
          Serial.print("  glide_duration_so_far_millis: ");
          Serial.print(glide_duration_so_far_millis);
          Serial.print("\n");
          Serial.print("  glide_percentage_progress: ");
          Serial.print(glide_percentage_progress);
          Serial.print("\n");
          Serial.print("  glide_frequency_distance: ");
          Serial.print(glide_frequency_distance);
          Serial.print("\n");
          Serial.print("  glide_hertz_to_add: ");
          Serial.print(glide_hertz_to_add);
          Serial.print("\n");
          Serial.print("  freq_after_pb_and_detune: ");
          Serial.print(freq_after_pb_and_detune);
          Serial.print("\n");
          Serial.print("  final_frequency: ");
          Serial.print(freq_after_pb_and_detune + glide_hertz_to_add);
          Serial.print("\n");
        }
      }
    }

    last_glide_update_micros = micros();
  }

  // if any notes are playing in `volume_modulation_mode`, we have to implement
  // ADSR stuff on our own.
  if (
    volume_modulation_mode_active &&
    (notes_playing[0].number != 0 || notes_playing[1].number != 0 || notes_playing[2].number != 0 ) &&
    ((time_in_micros - last_update) > update_every_micros)) {
    double volume = 0;
    int notes_playing_count = 0;
    for (int i = 0; i < 3; i++) {
      notes_playing[i].number != 0 && notes_playing_count++;
    }

    for (int i = 0; i < notes_playing_count; i++) {
      int note = notes_playing[i].number;

      if (note != 0) {
        double note_frequency = MIDI_NOTES_TO_FREQUENCIES[note];
        double temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
        note_frequency = note_frequency * pow(2, temp_double / 12.0);

        double yt = sine_waveform(note_frequency, time_in_seconds, 0.5, 0);
        yt *= linear_envelope(
          get_attack_seconds(i),
          get_decay_seconds(i),
          get_sustain_percent(i),
          get_release_seconds(i),
          (time_in_micros - notes_playing[i].on_time) / 1000000.0,
          -1
        );

        volume += (((yt + 0.5) * 15) / notes_playing_count);
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
    (notes_playing[0].number != 0 || notes_playing[1].number != 0 || notes_playing[2].number != 0 ) &&
    ((time_in_micros - last_update) > update_every_micros)) {
    int notes_playing_count = 0;
    for (int i = 0; i < 3; i++) {
      notes_playing[i].number != 0 && notes_playing_count++;
    }

    for (int i = 0; i < notes_playing_count; i++) {
      int note = notes_playing[i].number;
      double note_frequency = MIDI_NOTES_TO_FREQUENCIES[note];

      double temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      note_frequency = note_frequency * pow(2, temp_double / 12.0);

      if (note != 0) {
        double yt = sine_waveform(note_frequency, time_in_seconds, 0.5, 0);
        yt *= linear_envelope(
          get_attack_seconds(i),
          get_decay_seconds(i),
          get_sustain_percent(i),
          get_release_seconds(i),
          (time_in_micros - notes_playing[i].on_time) / 1000000.0,
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
