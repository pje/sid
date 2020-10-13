#include <math.h>
#include <usbmidi.h>
#include "util.h"

const int ARDUINO_SID_CHIP_SELECT_PIN = 13; // D13
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5; // D5

const double CLOCK_SIGNAL_FACTOR = 0.0596;

const byte REGISTER_BANK_OFFSET_VOICE_FREQUENCY_LO   = 0;
const byte REGISTER_BANK_OFFSET_VOICE_FREQUENCY_HI   = 1;
const byte REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_LO = 2;
const byte REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_HI = 3;
const byte REGISTER_BANK_OFFSET_VOICE_CONTROL        = 4;
const byte REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD    = 5;
const byte REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR    = 6;

const byte REGISTER_ADDRESS_FILTER_FREQUENCY_LO      = 21;
const byte REGISTER_ADDRESS_FILTER_FREQUENCY_HI      = 22;
const byte REGISTER_ADDRESS_FILTER_RESONANCE         = 23;
const byte REGISTER_ADDRESS_FILTER_MODE_VOLUME       = 24;

const byte SID_NOISE         = B10000000;
const byte SID_SQUARE        = B01000000;
const byte SID_RAMP          = B00100000;
const byte SID_TRIANGLE      = B00010000;
const byte SID_TEST          = B00001000;
const byte SID_RING          = B00000100;
const byte SID_SYNC          = B00000010;
const byte SID_3OFF          = B10000000;
const byte SID_FILTER_HP     = B01000000;
const byte SID_FILTER_BP     = B00100000;
const byte SID_FILTER_LP     = B00010000;
const byte SID_FILTER_OFF    = B00000000;
const byte SID_FILTER_VOICE1 = B00000001;
const byte SID_FILTER_VOICE2 = B00000010;
const byte SID_FILTER_VOICE3 = B00000100;
const byte SID_FILTER_EXT    = B00001000;

// since we have to set all the bits in a register byte at once,
// we must maintain a copy of the register's state so we don't clobber bits
// (SID actually has 28 registers but we don't use the last 4. hence 25)
byte sid_state_bytes[25] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

const byte MIDI_NOTE_ON        = B1001;
const byte MIDI_NOTE_OFF       = B1000;
const byte MIDI_PITCH_BEND     = B1110;
const byte MIDI_CONTROL_CHANGE = B1011;
const byte MIDI_PROGRAM_CHANGE = B1100;
const byte MIDI_TIMING_CLOCK   = B11111000;
const byte MIDI_CONTROL_CHANGE_BANK_SELECT = B00000000;

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_TRIANGLE   = 12; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_RAMP       = 13; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_SQUARE     = 14; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_NOISE      = 15; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE            = 16; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE                = 17; // 1-bit value
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
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE            = 35; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE       = 54; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_THREE   = 86; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE            = 55; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE             = 56; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE           = 57; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE           = 58; // 4-bit value

const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY              = 36; // 11-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE              = 37; // 4-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_LP             = 60; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_BP             = 61; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_HP             = 62; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE_OFF        = 63; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE              = 64; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO              = 72; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE            = 80; // 7-bit value
const byte MIDI_CONTROL_CHANGE_SET_VOLUME                        = 43; // 4-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_FOURTH_VOICE_MODE          = 81; // 1-bit value

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

const byte MAX_POLYPHONY = 3;
byte polyphony = 3;
byte notes_playing[MAX_POLYPHONY];
long note_on_times[MAX_POLYPHONY];

int midi_pitch_bend_max_semitones = 5;
double current_pitchbend_amount = 0.0; // [-1.0 .. 1.0]
double voice_detune_percents[MAX_POLYPHONY] = { 0.0, 0.0, 0.0 }; // [-1.0 .. 1.0]
int detune_max_semitones = 5;

const int DEFAULT_PULSE_WIDTH_VALUE = 2048; // (2**12 - 1) / 2
const int DEFAULT_WAVEFORM_VALUE = SID_TRIANGLE;
const int DEFAULT_ATTACK_VALUE = 0;
const int DEFAULT_DECAY_VALUE = 14;
const int DEFAULT_SUSTAIN_VALUE = 0;
const int DEFAULT_RELEASE_VALUE = 0;

// experimental: used to implement 14-bit resolution for PW values spread over two sequential CC messages
word pw_v1     = 0;
byte pw_v1_lsb = 0;
word pw_v2     = 0;
byte pw_v2_lsb = 0;
word pw_v3     = 0;
byte pw_v3_lsb = 0;

// used to implement RPN messages
word rpn_value = 0;
word data_entry = 0;

boolean volume_modulation_mode_active = false;

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

int num_presets = 0;

const double MIDI_NOTES_TO_FREQUENCIES[96] = {
  16.351597831287414,
  17.323914436054505,
  18.354047994837977,
  19.445436482630058,
  20.601722307054366,
  21.826764464562746,
  23.12465141947715,
  24.499714748859326,
  25.956543598746574,
  27.5,
  29.13523509488062,
  30.86770632850775,
  32.70319566257483,
  34.64782887210901,
  36.70809598967594,
  38.890872965260115,
  41.20344461410875,
  43.653528929125486,
  46.2493028389543,
  48.999429497718666,
  51.91308719749314,
  55.0,
  58.27047018976124,
  61.7354126570155,
  65.40639132514966,
  69.29565774421802,
  73.41619197935188,
  77.78174593052023,
  82.4068892282175,
  87.30705785825097,
  92.4986056779086,
  97.99885899543733,
  103.82617439498628,
  110.0,
  116.54094037952248,
  123.47082531403103,
  130.8127826502993,
  138.59131548843604,
  146.8323839587038,
  155.56349186104046,
  164.81377845643496,
  174.61411571650194,
  184.9972113558172,
  195.99771799087463,
  207.65234878997256,
  220.0,
  233.08188075904496,
  246.94165062806206,
  261.6255653005986,
  277.1826309768721,
  293.6647679174076,
  311.1269837220809,
  329.6275569128699,
  349.2282314330039,
  369.9944227116344,
  391.99543598174927,
  415.3046975799451,
  440.0,
  466.1637615180899,
  493.8833012561241,
  523.2511306011972,
  554.3652619537442,
  587.3295358348151,
  622.2539674441618,
  659.2551138257398,
  698.4564628660078,
  739.9888454232688,
  783.9908719634985,
  830.6093951598903,
  880.0,
  932.3275230361799,
  987.7666025122483,
  1046.5022612023945,
  1108.7305239074883,
  1174.6590716696303,
  1244.5079348883237,
  1318.5102276514797,
  1396.9129257320155,
  1479.9776908465376,
  1567.981743926997,
  1661.2187903197805,
  1760.0,
  1864.6550460723597,
  1975.533205024496,
  2093.004522404789,
  2217.4610478149766,
  2349.31814333926,
  2489.0158697766474,
  2637.02045530296,
  2793.825851464031,
  2959.955381693075,
  3135.9634878539946,
  3322.437580639561,
  3520.0,
  3729.3100921447194,
  3951.066410048992
};

void sid_transfer(byte address, byte data) {
  address &= B00011111;

  // PORTF is a weird 6-bit register (8 bits, but bits 2 and 3 don't exist)
  //
  // Port F Data Register — PORTF
  // bit  7    6    5    4    3    2    1    0
  //      F7   F6   F5   F4   -    -    F1   F0
  //
  // addr -    A4   A3   A2   -    -    A1   A0

  PORTF = ((address << 2) & B01110000) | (address & B00000011);
  PORTB = data;
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, LOW);
  // PORTC &= B01111111; // faster version of digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, LOW);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);
  sid_state_bytes[address] = data;
  // PORTC |= B10000000; // faster version of digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);
}

void sid_zero_all_registers() {
  for (byte i = 0; i < 25; i++) {
    sid_transfer(i, B00000000);
  }
}

void sid_zero_voice_registers(int voice) {
  byte address = 0;
  for (int i = 0; i < 7; i++) {
    address = (voice * 7) + i;
    sid_transfer(address, 0);
  }
}

void sid_set_volume(byte level) {
  byte address = REGISTER_ADDRESS_FILTER_MODE_VOLUME;
  byte data = (sid_state_bytes[address] & B11110000) | (level & B00001111);
  sid_transfer(address, data);
}

void sid_set_waveform(int voice, byte waveform) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = (waveform | (sid_state_bytes[address] & B00001111));
  sid_transfer(address, data);
}

void sid_set_ring_mod(int voice, boolean on) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data;
  if (on) {
    // ring mod repurposes the output of the triangle oscillator
    data = sid_state_bytes[address] | B00010100; // set triangle and ring mod bits, leave others as-is
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
    data = sid_state_bytes[address] & B11111011; // zero ring mod
  }
  sid_transfer(address, data);
}

void sid_set_test(int voice, boolean on) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = (on ? (sid_state_bytes[address] | SID_TEST) : (sid_state_bytes[address] & ~SID_TEST));
  sid_transfer(address, data);
}

void sid_set_sync(int voice, boolean on) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = (on ? (sid_state_bytes[address] | SID_SYNC) : (sid_state_bytes[address] & ~SID_SYNC));
  sid_transfer(address, data);
}

void sid_set_attack(int voice, byte attack) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & B00001111;
  data |= (attack << 4);
  sid_transfer(address, data);
}

void sid_set_decay(int voice, byte decay) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD;
  byte data = sid_state_bytes[address] & B11110000;
  data |= (decay & B00001111);
  sid_transfer(address, data);
}

void sid_set_sustain(int voice, byte sustain) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & B00001111;
  data |= (sustain << 4);
  sid_transfer(address, data);
}

void sid_set_release(int voice, byte release) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR;
  byte data = sid_state_bytes[address] & B11110000;
  data |= (release & B00001111);
  sid_transfer(address, data);
}

void sid_set_pulse_width(byte voice, word hertz) { // 12-bit value
  byte lo = lowByte(hertz);
  byte hi = highByte(hertz) & B00001111;
  byte address_lo = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_LO;
  byte address_hi = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_HI;
  sid_transfer(address_lo, lo);
  sid_transfer(address_hi, hi);
}

void sid_set_filter_frequency(word hertz) {
  byte lo = lowByte(hertz) & B00000111;
  byte hi = highByte(hertz << 5);
  sid_transfer(REGISTER_ADDRESS_FILTER_FREQUENCY_LO, lo);
  sid_transfer(REGISTER_ADDRESS_FILTER_FREQUENCY_HI, hi);
}

void sid_set_filter_resonance(byte hertz) {
  byte address = REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data = (sid_state_bytes[address] & B00001111) | (hertz << 4);
  sid_transfer(address, data);
}

void sid_set_filter(byte voice, boolean on) {
  byte address = REGISTER_ADDRESS_FILTER_RESONANCE;
  byte data;
  byte voice_filter_mask;
  if (voice == 0) {
    voice_filter_mask = SID_FILTER_VOICE1;
  } else if(voice == 1) {
    voice_filter_mask = SID_FILTER_VOICE2;
  } else if(voice == 2) {
    voice_filter_mask = SID_FILTER_VOICE3;
  }
  if (on) {
    data = sid_state_bytes[address] | voice_filter_mask;
  } else {
    data = sid_state_bytes[address] & ~voice_filter_mask;
  }
  sid_transfer(address, data);
}

// filter modes are additive (e.g. you can set LP & HP simultaneously)
void sid_set_filter_mode(byte mode, boolean on) {
  byte address = REGISTER_ADDRESS_FILTER_MODE_VOLUME;
  byte data;
  if (on) {
    data = (sid_state_bytes[address] & B00001111) | mode;
  } else {
    data = (sid_state_bytes[address] & B00001111) & ~mode;
  }
  sid_transfer(address, data);
}

void sid_set_voice_frequency(int voice, word hertz) {
  word frequency = (hertz / CLOCK_SIGNAL_FACTOR);
  byte loFrequency = lowByte(frequency);
  byte hiFrequency = highByte(frequency);
  byte loAddress = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_FREQUENCY_LO;
  byte hiAddress = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_FREQUENCY_HI;
  sid_transfer(loAddress, loFrequency);
  sid_transfer(hiAddress, hiFrequency);
}

void sid_set_gate(int voice, boolean state) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data;
  if (state) {
    data = sid_state_bytes[address] | B00000001;
  } else {
    data = sid_state_bytes[address] & B11111110;
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
    notes_playing[i] = NULL;
  }
}

void handle_message_voice_attack_change(byte voice, byte envelope_value) {
  sid_set_attack(voice, envelope_value);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_attack(i, envelope_value);
    }
  }
}

void handle_message_voice_decay_change(byte voice, byte envelope_value) {
  sid_set_decay(voice, envelope_value);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_decay(i, envelope_value);
    }
  }
}

void handle_message_voice_sustain_change(byte voice, byte envelope_value) {
  sid_set_sustain(voice, envelope_value);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_sustain(i, envelope_value);
    }
  }
}

void handle_message_voice_release_change(byte voice, byte envelope_value) {
  sid_set_release(voice, envelope_value);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_release(i, envelope_value);
    }
  }
}

void handle_message_voice_waveform_change(byte voice, byte waveform, boolean on) {
  if (on) {
    sid_set_waveform(voice, waveform);

    if (polyphony == 3) {
      for (int i = 0; i < 3; i++) {
        sid_set_waveform(i, waveform);
      }
    }
  } else { // we turn the voice "off" by zeroing its waveform bits and gate bit
    sid_set_waveform(voice, 0);
    sid_set_gate(voice, false);

    if (polyphony == 3) {
      for (int i = 0; i < 3; i++) {
        sid_set_waveform(i, 0);
        sid_set_gate(i, false);
      }
    }
  }
}

void handle_message_voice_filter_change(byte voice, boolean on) {
  sid_set_filter(voice, on);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_filter(i, on);
    }
  }
}

void handle_message_voice_pulse_width_change(byte voice, word frequency) {
  sid_set_pulse_width(voice, frequency);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_pulse_width(i, frequency);
    }
  }
}

void handle_message_voice_ring_mod_change(byte voice, boolean on) {
  sid_set_ring_mod(voice, on);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_ring_mod(voice, on);
    }
  }
}

void handle_message_voice_sync_change(byte voice, boolean on) {
  sid_set_sync(voice, on);

  if (polyphony == 3) {
    for (int i = 0; i < 3; i++) {
      sid_set_sync(voice, on);
    }
  }
}

void handle_message_voice_detune_change(byte voice, double detune_factor) {
  voice_detune_percents[voice] = detune_factor;
  double semitone_change = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[voice] * detune_max_semitones);
  double frequency = MIDI_NOTES_TO_FREQUENCIES[notes_playing[voice]] * pow(2, semitone_change / 12.0);
  sid_set_voice_frequency(voice, (word)frequency);
}

void handle_message_note_on(byte note_number, byte velocity) {
  double temp_double = 0.0;
  int note_voice = 0;
  if (polyphony == 1) { // we're mono. for now all voices will have the same frequency.
    for (int i = 0; i < MAX_POLYPHONY; i++ ) {
      temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      temp_double = MIDI_NOTES_TO_FREQUENCIES[note_number] * pow(2, temp_double / 12.0);
      if (!volume_modulation_mode_active) {
        sid_set_gate(i, false);
        sid_set_voice_frequency(i, (word)temp_double);
        sid_set_gate(i, true);
      }
      notes_playing[i] = note_number;
      note_on_times[i] = micros();
    }
  } else {  // we're poly, so every voice has to have its own frequency
    for (int i = 0; i < polyphony; i++) {
      if (notes_playing[i] == NULL) {
        notes_playing[i] = note_number;
        note_on_times[i] = micros();
        note_voice = i;
        break;
      } else {
        if (i == (polyphony - 1)) {
          notes_playing[0] = notes_playing[1];
          notes_playing[1] = notes_playing[2];
          notes_playing[2] = note_number;

          note_on_times[0] = note_on_times[1];
          note_on_times[1] = note_on_times[2];
          note_on_times[2] = micros();

          note_voice = i;
          break;
        }
      }
    }
    temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[note_voice] * detune_max_semitones);
    temp_double = MIDI_NOTES_TO_FREQUENCIES[note_number] * pow(2, temp_double / 12.0);
    if (!volume_modulation_mode_active) {
      sid_set_gate(note_voice, false);
      sid_set_voice_frequency(note_voice, (word)temp_double);
      sid_set_gate(note_voice, true);
    }
  }
}

void handle_message_note_off(byte note_number, byte velocity) {
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    if (notes_playing[i] == note_number) {
      notes_playing[i] = NULL;
      note_on_times[i] = NULL;
      sid_set_gate(i, false);
    }
  }
}

void handle_message_pitchbend_change(word pitchbend) {
  double temp_double = 0.0;
  current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the "neutral" pitchbend value (half of 2**14)
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    if (notes_playing[i] != NULL) {
      temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      temp_double = MIDI_NOTES_TO_FREQUENCIES[notes_playing[i]] * pow(2, temp_double / 12.0);
      sid_set_voice_frequency(i, (word)temp_double);
    }
  }
}

void handle_message_program_change(byte program_number) {
  switch (program_number) {
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_POLYPHONIC:
    polyphony = 3;
    break;
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC:
    polyphony = 1;
    break;
  }
}

void handle_message_bank_select(byte bank_number) {
  if (bank_number <= num_presets) {
  }
}

void handle_state_dump_request() {
  // for testing purposes, we'll print the state of our sid representation
  // TODO: this should also print the state of our 'virtual' controls
  // voice detune, LFOs (potentially)
  for (int i = 0; i < 25; i++) {
    Serial.print(sid_state_bytes[i], BIN);
    Serial.println("");
  }
}

void setup() {
  delay(1000);
  // without this the micro locks up on startup and requires a
  // reset button push? (only when not connected to USB), which is the use-case

  DDRF = B01110011; // initialize 5 PORTF pins as output (connected to A0-A4)
  DDRB = B11111111; // initialize 8 PORTB pins as output (connected to D0-D7)
  // technically SID allows us to read from its last 4 registers, but we don't
  // need to, so we just always keep SID's R/W pin low (signifying "write"),
  // which seems to work ok

  polyphony = 3;
  nullify_notes_playing();

  start_clock();

  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  sid_zero_all_registers();
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    sid_set_pulse_width(i, DEFAULT_PULSE_WIDTH_VALUE);
    sid_set_waveform(i, DEFAULT_WAVEFORM_VALUE);
    sid_set_attack(i, DEFAULT_ATTACK_VALUE);
    sid_set_decay(i, DEFAULT_DECAY_VALUE);
    sid_set_sustain(i, DEFAULT_SUSTAIN_VALUE);
    sid_set_release(i, DEFAULT_RELEASE_VALUE);
  }
  sid_set_volume(15);

  Serial.begin(31250);
}

void handle_midi_input(Stream *midi_port) {
  if (midi_port->available() > 0) {
    byte incomingByte = midi_port->read();
    byte opcode  = incomingByte >> 4;
    byte channel = incomingByte & (B00001111);
    byte data_byte_one = 0;
    byte data_byte_two = 0;
    int note_voice = 0;
    word pitchbend = 8192.0;
    byte controller_number = 0;
    byte controller_value = 0;
    double temp_double = 0.0;

    if (channel == MIDI_CHANNEL && opcode >= B1000 && opcode <= B1110) { // Voice/Mode Messages, on our channel
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

        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE:
          temp_double = (controller_value / 127.0) * 4095.0;
          pw_v1 = ((word)temp_double) + pw_v1_lsb;
          handle_message_voice_pulse_width_change(0, pw_v1);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO:
          temp_double = (controller_value / 127.0) * 4095.0;
          pw_v2 = ((word)temp_double) + pw_v2_lsb;
          handle_message_voice_pulse_width_change(1, pw_v2);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE:
          temp_double = (controller_value / 127.0) * 4095.0;
          pw_v3 = ((word)temp_double) + pw_v3_lsb;
          handle_message_voice_pulse_width_change(2, pw_v3);
          break;

        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_ONE:
          pw_v1_lsb = controller_value & 0b00011111;
          pw_v1 += pw_v1_lsb;
          handle_message_voice_pulse_width_change(0, pw_v1);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_TWO:
          pw_v2_lsb = controller_value & 0b00011111;
          pw_v2 += pw_v2_lsb;
          handle_message_voice_pulse_width_change(1, pw_v2);
          break;
        case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_THREE:
          pw_v3_lsb = controller_value & 0b00011111;
          pw_v3 += pw_v3_lsb;
          handle_message_voice_pulse_width_change(2, pw_v3);
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
          temp_double = ((controller_value / 64.0) - 1);
          handle_message_voice_detune_change(0, temp_double);
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO:
          temp_double = ((controller_value / 64.0) - 1);
          handle_message_voice_detune_change(1, temp_double);
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE:
          temp_double = ((controller_value / 64.0) - 1);
          handle_message_voice_detune_change(2, temp_double);
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
          temp_double = (controller_value / 127.0) * 2047.0;
          sid_set_filter_frequency((word)temp_double);
          break;
        case MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE:
          controller_value >> 3;
          sid_set_filter_resonance((byte)temp_double);
          break;

        case MIDI_CONTROL_CHANGE_SET_VOLUME:
          temp_double = (controller_value / 127.0) * 15;
          sid_set_volume((word)temp_double);
          break;

        case MIDI_CONTROL_CHANGE_BANK_SELECT:
          handle_message_bank_select(controller_value);
          break;

        case MIDI_CONTROL_CHANGE_RPN_LSB:
          rpn_value += (controller_value & 0b01111111);
          break;

        case MIDI_CONTROL_CHANGE_RPN_MSB:
          temp_double = (controller_value / 127.0) * 4095.0;
          rpn_value = (word)temp_double;
          break;

        case MIDI_CONTROL_CHANGE_DATA_ENTRY:
          data_entry = controller_value;
          if (rpn_value == MIDI_RPN_PITCH_BEND_SENSITIVITY) {
            midi_pitch_bend_max_semitones = data_entry;
            detune_max_semitones = data_entry;
          }
          break;

        case MIDI_CONTROL_CHANGE_TOGGLE_FOURTH_VOICE_MODE:
          volume_modulation_mode_active = (controller_value == 127);
          break;

        case 127:
          if (controller_value == 127) {
            handle_state_dump_request();
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
        data_byte_two = midi_port->read();
        if (data_byte_one < 96) { // SID can't handle freqs > B7
          handle_message_note_on(data_byte_one, data_byte_two);
        }
        break;
      case MIDI_NOTE_OFF:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read();
        handle_message_note_off(data_byte_one, data_byte_two);
        break;
      }
    }
  }
}

long last_update = 0;
const double update_every_micros = (100.0 / 4.41);

byte lowNibble(byte b) { return(b & B00001111); }
byte highNibble(byte b) { return((b & B11110000) >> 4); }

double get_attack_seconds(unsigned int voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_attack_values_to_seconds[value]);
}

double get_decay_seconds(unsigned int voice) {
  byte value = lowNibble(sid_state_bytes[(voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD]);
  return(sid_decay_and_release_values_to_seconds[value]);
}

// returns float [0..1]
double get_sustain_percent(unsigned int voice) {
  byte value = highNibble(sid_state_bytes[(voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR]);
  return((double) value / 16.0);
}

double get_release_seconds(unsigned int voice) {
  byte value = lowNibble(sid_state_bytes[(voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR]);
  return(sid_decay_and_release_values_to_seconds[value]);
}

void loop () {
  long time_in_micros = micros();

  if (volume_modulation_mode_active && notes_playing[0] != NULL && ((time_in_micros - last_update) > update_every_micros)) {
    double volume = 0;
    double yt = 0;
    double seconds = time_in_micros / 1000000.0;
    int notes_playing_count = 0;
    for (int i = 0; i < 3; i++) {
      notes_playing[i] != NULL && notes_playing_count++;
    }

    for (int i = 0; i < notes_playing_count; i++) {
      int note = notes_playing[i];

      if (note != NULL) {
        double yt = sine_waveform(MIDI_NOTES_TO_FREQUENCIES[note], seconds, 0.5, 0);
        yt *= linear_envelope(
          get_attack_seconds(i),
          get_decay_seconds(i),
          get_sustain_percent(i),
          get_release_seconds(i),
          (time_in_micros - note_on_times[i]) / 1000000.0,
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

  USBMIDI.poll();

  handle_midi_input(&USBMIDI);
  handle_midi_input(&Serial1);
}
