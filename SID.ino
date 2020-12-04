#include <Arduino.h>
#include <MemoryFree.h>
#include <math.h>
#include <usbmidi.h>
#include "src/deque.h"
#include "src/hash_table.h"
#include "src/midi_constants.h"
#include "src/note.h"
#include "src/sid.h"
#include "src/stdinout.h"
#include "src/util.h"

#define DEBUG_LOGGING true

const unsigned int deque_size = 32; // the number of notes that can be held simultaneously
const int ARDUINO_SID_CHIP_SELECT_PIN = 13; // wired to SID's CS pin
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5; // wired to SID's Ø2 pin
const byte MAX_POLYPHONY = 3;
const byte DEFAULT_PITCH_BEND_SEMITONES = 5;
const float DEFAULT_GLIDE_TIME_MILLIS = 100.0;
const unsigned int GLIDE_TIME_MIN_MILLIS = 1;
const unsigned int GLIDE_TIME_MAX_MILLIS = 10000;
const word MAX_PULSE_WIDTH_VALUE = 4095;
const word DEFAULT_PULSE_WIDTH = 2048; // (2**12 - 1) / 2
const byte DEFAULT_WAVEFORM = SID_SQUARE;
const byte DEFAULT_ATTACK = 0;
const byte DEFAULT_DECAY = 0;
const byte DEFAULT_SUSTAIN = 15;
const byte DEFAULT_RELEASE = 0;
const unsigned short int DEFAULT_FILTER_FREQUENCY = 1000;
const byte DEFAULT_FILTER_RESONANCE = 15;
const byte DEFAULT_VOLUME = 15;
const unsigned int PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY = 65535;
const float UPDATE_EVERY_MICROS = (100.0 / 4.41);

byte polyphony = 1;
word glide_time_raw_word;
float glide_time_millis = DEFAULT_GLIDE_TIME_MILLIS;
bool legato_mode = (polyphony == 1) && glide_time_millis > 0;
byte glide_time_raw_lsb;
unsigned long glide_start_time_micros = 0;
byte glide_to = 0;
byte glide_from = 0;
int midi_pitch_bend_max_semitones = DEFAULT_PITCH_BEND_SEMITONES;
float current_pitchbend_amount = 0.0; // [-1.0 .. 1.0]
byte detune_max_semitones = 5;
// temp vars for implementing 14-bit midi CC messages spread over two messages
word pw_v1     = DEFAULT_PULSE_WIDTH;
byte pw_v1_lsb = 0;
word pw_v2     = DEFAULT_PULSE_WIDTH;
byte pw_v2_lsb = 0;
word pw_v3     = DEFAULT_PULSE_WIDTH;
byte pw_v3_lsb = 0;
word detune_v1_raw_word = 8192;
byte detune_v1_lsb      = 0;
word detune_v2_raw_word = 8192;
byte detune_v2_lsb      = 0;
word detune_v3_raw_word = 8192;
byte detune_v3_lsb      = 0;
word filter_frequency = 0;
byte filter_frequency_lsb = 0;
word rpn_value = 0; // used to implement RPN messages
word data_entry = 0; // used to implement RPN messages

// You know that click when we change the SID's volume? Turns out if we modulate
// that click we can generate arbitrary 4-bit wveforms including sine waves and
// sample playback. (Currently this mode just means "4-bit sine")
bool volume_modulation_mode_active = false;
bool pulse_width_modulation_mode_active = false;

unsigned long last_update = 0;
unsigned long last_glide_update_micros = 0;
unsigned long time_in_micros = 0;
unsigned long time_in_seconds = 0;

struct note oscillator_notes[3] = { { .number=0, .on_time=0, .off_time=0 } };
float voice_detune_percents[MAX_POLYPHONY] = { 0.0, 0.0, 0.0 }; // [-1.0 .. 1.0]
deque *notes = deque_initialize(deque_size, stdout, _note_indexer, _note_node_print_function);

static char float_string[15];

void clean_slate();
void update_oscillator_frequencies();

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
  for (unsigned char i = 0; i < MAX_POLYPHONY; i++) {
    oscillator_notes[i] = { .number = 0, .on_time = 0, .off_time = 0 };
  }

  deque_empty(notes);
}

void handle_voice_attack_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < polyphony; i++) {
      sid_set_attack(i, envelope_value);
    }
  } else {
    sid_set_attack(voice, envelope_value);
  }
}

void handle_voice_decay_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < 3; i++) {
      sid_set_decay(i, envelope_value);
    }
  } else {
    sid_set_decay(voice, envelope_value);
  }
}

void handle_voice_sustain_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < 3; i++) {
      sid_set_sustain(i, envelope_value);
    }
  } else {
    sid_set_sustain(voice, envelope_value);
  }
}

void handle_voice_release_change(byte voice, byte envelope_value) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < 3; i++) {
      sid_set_release(i, envelope_value);
    }
  } else {
    sid_set_release(voice, envelope_value);
  }
}

void handle_voice_waveform_change(byte voice, byte waveform, bool on) {
  if (polyphony == 1) {
    sid_toggle_waveform(voice, waveform, on);
  } else {
    for (unsigned char i = 0; i < 3; i++) {
      sid_toggle_waveform(i, waveform, on);
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

    sid_transfer(address, data);
  } else {
    sid_set_filter(voice, on);
  }
}

void handle_voice_pulse_width_change(byte voice, word frequency) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < polyphony; i++) {
      sid_set_pulse_width(i, frequency);
    }
  } else {
    sid_set_pulse_width(voice, frequency);
  }
}

void handle_voice_ring_mod_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < polyphony; i++) {
      sid_set_ring_mod(i, on);
    }
  } else {
    sid_set_ring_mod(voice, on);
  }
}

void handle_voice_sync_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < polyphony; i++) {
      sid_set_sync(i, on);
    }
  } else {
    sid_set_sync(voice, on);
  }
}

void handle_voice_test_change(byte voice, bool on) {
  if (polyphony > 1) {
    for (unsigned char i = 0; i < polyphony; i++) {
      sid_set_test(i, on);
    }
  } else {
    sid_set_test(voice, on);
  }
}

// detune factor: [-1.0...1.0]
void handle_voice_detune_change(byte voice, float detune_factor) {
  voice_detune_percents[voice] = detune_factor;
  update_oscillator_frequencies();
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
    if (get_voice_gate(voice)) { // this voice is already playing another note, still in its ADS phase. So glide might be relevant. Otherwise we can just clobber the gate
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
  deque_append_replace(notes, { .number=note_number, .on_time=now, .off_time=0, .voiced_by_oscillator=voice });

  oscillator_notes[voice].number = note_number;
  oscillator_notes[voice].off_time = 0;
}

void log_load_stats() {
  printf(
    "{free_mem: %d, h(%u/%u){load: %d%%, coll: %d%%}\n",
    freeMemory(),
    notes->ht->size,
    notes->ht->max_size,
    (unsigned int)(hash_table_load_factor(notes->ht) * 100),
    (unsigned int)(hash_table_collision_ratio(notes->ht) * 100)
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
    for (unsigned char i = 0; i < MAX_POLYPHONY; i++ ) {
      if (get_voice_waveform(i) != 0) { // don't even try to play "muted" voices
        play_note_for_voice(note_number, i);
      }
    }
    return;
  }

  // we're poly, so every voice has its own frequency.
  // if there's a free voice, we can just use that.
  for (unsigned char i = 0; i < polyphony; i++) {
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

  bool remove_note = false;

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
        remove_note = true;
      } else {
        sid_set_gate(i, false);
        oscillator_notes[i].off_time = now;
      }
    } else {
      // if the note is not being voiced, we may as well try to remove its entry from the deque now
      remove_note = true;
    }
  }

  if (remove_note) {
    deque_remove_by_key(notes, note_number);
  }

  if (note_number == glide_to) {
    glide_from = 0;
    glide_to = 0;
  }
}

void handle_pitchbend_change(word pitchbend) {
  float temp_float = 0.0;
  current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the "neutral" pitchbend value (half of 2**14)
  for (unsigned char i = 0; i < MAX_POLYPHONY; i++) {
    if (oscillator_notes[i].number != 0) {
      temp_double = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      temp_double = note_number_to_frequency(oscillator_notes[i].number) * pow(2, temp_double / 12.0);
      if (!pulse_width_modulation_mode_active){
        sid_set_voice_frequency(i, temp_double);
      }
    }
  }
}

void duplicate_voice(unsigned char from_voice, unsigned char to_voice) {
  for (unsigned char i = 0; i < 7; i++) {
    sid_transfer((to_voice * 7) + i, sid_state_bytes[(from_voice * 7) + i]);
  }

  voice_detune_percents[to_voice] = voice_detune_percents[from_voice];
}

void initialize_glide_state() {
  glide_time_raw_word = 0;
  glide_time_raw_lsb = 0;
  glide_start_time_micros = 0;
  glide_to = 0;
  glide_from = 0;
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
    for (unsigned char i = 0; i < 3; i++) {
      oscillator_notes[i] = { .number = 0, .on_time = 0, .off_time = 0 };
    }
    legato_mode = false;
    break;
  case MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC_UNISON:
    polyphony = 1;
    initialize_glide_state();
    legato_mode = (polyphony == 1) && (glide_time_millis > 0.01);
    break;
  case MIDI_PROGRAM_CHANGE_HARDWARE_RESET:
    clean_slate();
    break;
  }
}

void enable_pulse_width_modulation_mode() {
  pulse_width_modulation_mode_active = true;

  for (unsigned char i = 0; i < 3; i++) {
    sid_set_waveform(i, SID_SQUARE, true);
    sid_set_voice_frequency(i, PULSE_WIDTH_MODULATION_MODE_CARRIER_FREQUENCY);
  }
}

void reset_voice_waveforms_to_default() {
  for (unsigned int i = 0; i < polyphony; i++) {
    if (i < polyphony) {
      sid_set_waveform(i, DEFAULT_WAVEFORM, true);
    } else {
      sid_zero_waveform(i);
    }
  }
}

void disable_pulse_width_modulation_mode() {
  pulse_width_modulation_mode_active = false;

  for (unsigned char i = 0; i < 3; i++) {
    sid_set_test(i, false);
  }

  reset_voice_waveforms_to_default();
}

void handle_state_dump_request(bool human) {
  // for testing purposes, print the state of our sid representation, which
  // should (hopefully) mirror what the SID's registers are right now

  if (human) {
    #if DEBUG_LOGGING
      printf("V#  WAVE     FREQ    DTUN   A      D      S    R      PW   TEST RING SYNC GATE FILT\n");

      for (unsigned int i = 0; i < 3; i++) {
        printf("V%u  ", i);

        byte wave = get_voice_waveform(i) << 4;

        if (wave == 0) {
          printf("Disabled");
        } else {
          printf(((wave & SID_TRIANGLE) != 0) ? "Tr" : "  ");
          printf(((wave & SID_RAMP)     != 0) ? "Rm" : "  ");
          printf(((wave & SID_SQUARE)   != 0) ? "Sq" : "  ");
          printf(((wave & SID_NOISE)    != 0) ? "Ns" : "  ");
        }

        char float_string[] = "       ";

        float f = get_voice_frequency(i);

        float_as_padded_string(float_string, f, 4, 2, '0');
        printf(" %s", float_string);

        f = voice_detune_percents[i] * 100;
        float_as_padded_string(float_string, f, 3, 1, '0');
        printf(" %s%%", float_string);

        float a = get_attack_seconds(i);
        float d = get_decay_seconds(i);
        float s = get_sustain_percent(i);
        float r = get_release_seconds(i);

        float_as_padded_string(float_string, a, 2, 3, '0');
        printf(" %s", float_string);

        float_as_padded_string(float_string, d, 2, 3, '0');
        printf(" %s", float_string);

        sprintf(float_string, "%3d", (int)(s * 100));
        printf(" %s%%", float_string);

        float_as_padded_string(float_string, r, 2, 3, '0');
        printf(" %s", float_string);

        printf(
          " %4u  %d    %d    %d    %d    %d\n",
          get_voice_pulse_width(i),
          get_voice_test_bit(i),
          get_voice_ring_mod(i),
          get_voice_sync(i),
          get_voice_gate(i),
          get_filter_enabled_for_voice(i)
        );
      }

      printf("Filter frequency: %u resonance: %u mode: ", get_filter_frequency(), get_filter_resonance());

      byte mask = sid_state_bytes[SID_REGISTER_ADDRESS_FILTER_MODE_VOLUME] & 0B01110000;

      printf("%s", mask & SID_FILTER_HP ? "HP" : "--");
      printf("%s", mask & SID_FILTER_BP ? "BP" : "--");
      printf("%s", mask & SID_FILTER_LP ? "LP" : "--");
      printf("\n");

      printf("Global Mode: %s\n", polyphony == 1 ? "Mono Unison" : "Paraphonic");
      if (polyphony == 1) {
        printf("Glide enabled: %s", legato_mode ? "true" : "false");
        char str[12];
        float_as_padded_string(str, glide_time_millis, 2, 3, '0');
        printf("\nGlide time: %s\n", str);
      }
      if (volume_modulation_mode_active) {
        printf(" <volume modulation mode>\n");
      } else if (pulse_width_modulation_mode_active) {
        printf(" <pulse width modulation mode>\n");
      }

      printf("Volume: %u\n", get_volume());
    #endif

    inspect_oscillator_notes();
    deque_inspect(notes);
  } else {
    for (unsigned char i = 0; i < 25; i++) {
      print_byte_in_binary(sid_state_bytes[i]);
    }
  }

  log_load_stats();
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
          printf("[%lu] Received MIDI CC %u %u\n", time_in_micros, controller_number, controller_value);
        #endif

        switch (controller_number) {
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_SQUARE:
          handle_voice_waveform_change(0, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_SQUARE:
          handle_voice_waveform_change(1, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_SQUARE:
          handle_voice_waveform_change(2, SID_SQUARE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_TRIANGLE:
          handle_voice_waveform_change(0, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_TRIANGLE:
          handle_voice_waveform_change(1, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_TRIANGLE:
          handle_voice_waveform_change(2, SID_TRIANGLE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_RAMP:
          handle_voice_waveform_change(0, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_RAMP:
          handle_voice_waveform_change(1, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_RAMP:
          handle_voice_waveform_change(2, SID_RAMP, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_NOISE:
          handle_voice_waveform_change(0, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_NOISE:
          handle_voice_waveform_change(1, SID_NOISE, controller_value == 127);
          break;
        case MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_NOISE:
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

        // LSB messages will not trigger change on the SID!
        // must be followed up by a MSB message to trigger a SID update w/ both
        case MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_ONE:
          detune_v1_lsb = controller_value & 0B01111111;
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_TWO:
          detune_v2_lsb = controller_value & 0B01111111;
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_THREE:
          detune_v3_lsb = controller_value & 0B01111111;
          break;

        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE:
          detune_v1_raw_word = ((word)controller_value & 0B01111111) << 7;
          detune_v1_raw_word += detune_v1_lsb;
          handle_voice_detune_change(0, ((detune_v1_raw_word / 16383.0) * 2 - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO:
          detune_v2_raw_word = ((word)controller_value & 0B01111111) << 7;
          detune_v2_raw_word += detune_v2_lsb;
          handle_voice_detune_change(1, ((detune_v2_raw_word / 16383.0) * 2 - 1));
          break;
        case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE:
          detune_v3_raw_word = ((word)controller_value & 0B01111111) << 7;
          detune_v3_raw_word += detune_v3_lsb;
          handle_voice_detune_change(2, ((detune_v3_raw_word / 16383.0) * 2 - 1));
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
          if (glide_time_millis <= GLIDE_TIME_MIN_MILLIS) {
            glide_time_millis = 0;
          }
          legato_mode = (polyphony == 1) && (glide_time_millis > 0.01);
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
          printf("[%lu] Received MIDI PC %u\n", time_in_micros, data_byte_one);
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
          printf("[%lu] Received MIDI PB %u\n", time_in_micros, pitchbend);
        #endif

        handle_pitchbend_change(pitchbend);
        break;
      case MIDI_NOTE_ON:
        while (midi_port->available() <= 0) {}
        data_byte_one = midi_port->read();
        while (midi_port->available() <= 0) {}
        data_byte_two = midi_port->read(); // "velocity", which we don't use

        #if DEBUG_LOGGING
          printf("[%lu] Received MIDI Note On %u\n", time_in_micros, data_byte_one);
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
          printf("[%lu] Received MIDI Note Off %u\n", time_in_micros, data_byte_one);
        #endif

        if (data_byte_one < 96) { // SID can't handle freqs > B7
          handle_note_off(data_byte_one);
        }
        break;
      }
    }
  }
}

// manually update oscillator frequencies to account for glide times
void update_oscillator_frequencies() {
  unsigned long glide_duration_so_far_millis = (time_in_micros - glide_start_time_micros) / 1000;
  if (glide_time_millis > 0) {
  float glide_percentage_progress;
    glide_percentage_progress = glide_duration_so_far_millis / glide_time_millis;
  } else {
    glide_percentage_progress = 1.0;
  }
  float glide_frequency_distance = 0.0;
  float glide_hertz_to_add = 0.0;
  float from_hertz = 0.0;
  float to_hertz = 0.0;
  float semitones_bent = 0.0;

  for (unsigned char i = 0; i < MAX_POLYPHONY; i++) {
    byte voice_note = oscillator_notes[i].number;
    if (voice_note != 0) {
      byte note_target = glide_time_millis > 0 && glide_to != 0 ? glide_to : voice_note;
      semitones_bent = (current_pitchbend_amount * midi_pitch_bend_max_semitones) + (voice_detune_percents[i] * detune_max_semitones);
      to_hertz = note_number_to_frequency(note_target) * pow(2, semitones_bent / 12.0);

      if (glide_percentage_progress >= 1.0) { // don't over-glide
        sid_set_voice_frequency(i, to_hertz);
        last_glide_update_micros = micros();
        continue;
      }

      from_hertz = note_number_to_frequency(glide_from) * pow(2, semitones_bent / 12.0);
      glide_frequency_distance = to_hertz - from_hertz;
      glide_hertz_to_add = glide_frequency_distance * glide_percentage_progress;
      sid_set_voice_frequency(i, from_hertz + glide_hertz_to_add);
      last_glide_update_micros = micros();
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
    unsigned int db_bytes = sizeof(maybe_hash_table_element) * notes->ht->max_size;

    printf("bytes allocated for deque/hash: %u + %u + %u = %u\n", dq_bytes, ht_bytes, db_bytes, dq_bytes + ht_bytes + db_bytes);
  #endif

  initialize_glide_state();
  polyphony = 1;
  glide_time_millis = DEFAULT_GLIDE_TIME_MILLIS;
  legato_mode = (polyphony == 1) && (glide_time_millis > 0.01);
  midi_pitch_bend_max_semitones = 5;
  current_pitchbend_amount = 0.0;
  detune_max_semitones = 5;
  pw_v1     = DEFAULT_PULSE_WIDTH;
  pw_v1_lsb = 0;
  pw_v2     = DEFAULT_PULSE_WIDTH;
  pw_v2_lsb = 0;
  pw_v3     = DEFAULT_PULSE_WIDTH;
  pw_v3_lsb = 0;
  detune_v1_raw_word = 8192;
  detune_v1_lsb      = 0;
  detune_v2_raw_word = 8192;
  detune_v2_lsb      = 0;
  detune_v3_raw_word = 8192;
  detune_v3_lsb      = 0;
  filter_frequency = DEFAULT_FILTER_FREQUENCY;
  filter_frequency_lsb = 0;
  rpn_value = 0;
  data_entry = 0;
  volume_modulation_mode_active = false;
  pulse_width_modulation_mode_active = false;
  last_update = 0;

  sid_zero_all_registers();
  reset_voice_waveforms_to_default();
  for (unsigned char i = 0; i < MAX_POLYPHONY; i++) {
    sid_set_pulse_width(i, DEFAULT_PULSE_WIDTH);
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
  for (unsigned char i = 0; i < 3; i++) {
    if (oscillator_notes[i].off_time > 0 && (time_in_micros > (oscillator_notes[i].off_time + get_release_seconds(i) * 1000000.0))) {
      // we're past the release phase, so the voice can't be making any noise, so we must "fully" silence it
      sid_set_voice_frequency(i, 0);
      oscillator_notes[i].on_time = 0;
      oscillator_notes[i].off_time = 0;

      #if DEBUG_LOGGING
        printf("leak detector deleted note: %u\n", oscillator_notes[i].number);
      #endif

      deque_remove_by_key(notes, oscillator_notes[i].number);
      oscillator_notes[i].number = 0;
    }
  }

  if (legato_mode &&
      glide_time_millis > 0 &&
      (oscillator_notes[0].number != 0 || oscillator_notes[1].number != 0 || oscillator_notes[2].number != 0 ) &&
      ((last_glide_update_micros + UPDATE_EVERY_MICROS) <= time_in_micros))
  {
    update_oscillator_frequencies();
  }

  // if any notes are playing in `volume_modulation_mode`, we have to implement
  // ADSR stuff on our own.
  if (
    volume_modulation_mode_active &&
    (oscillator_notes[0].number != 0 || oscillator_notes[1].number != 0 || oscillator_notes[2].number != 0 ) &&
    ((time_in_micros - last_update) > UPDATE_EVERY_MICROS)) {
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
    ((time_in_micros - last_update) > UPDATE_EVERY_MICROS)) {
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
