#ifndef SRC_MIDI_CONSTANTS_H
#define SRC_MIDI_CONSTANTS_H

#include "util.h"

const byte MIDI_NOTE_ON        = 0B1001;
const byte MIDI_NOTE_OFF       = 0B1000;
const byte MIDI_PITCH_BEND     = 0B1110;
const byte MIDI_CONTROL_CHANGE = 0B1011;
const byte MIDI_PROGRAM_CHANGE = 0B1100;
const byte MIDI_TIMING_CLOCK   = 0B11111000;

const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_TRIANGLE   = 12; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_RAMP       = 13; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_SQUARE     = 14; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_ONE_NOISE      = 15; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE               = 16; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE                   = 17; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_ONE                   = 82; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_ONE                 = 18; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE            = 44; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_ONE        = 70; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_ONE                 = 39; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE                  = 40; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE                = 41; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE                = 42; // 4-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_TRIANGLE   = 20; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_RAMP       = 21; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_SQUARE     = 22; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_TWO_NOISE      = 23; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO               = 24; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO                   = 25; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_TWO                   = 90; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_TWO                 = 26; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO            = 46; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_TWO        = 78; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO                 = 47; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO                  = 48; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO                = 49; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO                = 50; // 4-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_TRIANGLE = 28; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_RAMP     = 29; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_SQUARE   = 30; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_WAVEFORM_VOICE_THREE_NOISE    = 31; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE             = 33; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE                 = 34; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_TEST_VOICE_THREE                 = 98; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE               = 35; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE          = 54; // 7-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_LSB_VOICE_THREE      = 86; // 5-bit value (12-bit total)
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE               = 55; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE                = 56; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE              = 57; // 4-bit value
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE              = 58; // 4-bit value

const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY                 = 36; // 7-bit value (11-bit total)
const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY_LSB             = 68; // 4-bit value (11-bit total)
const byte MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE                 = 37; // 4-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_LP                = 60; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_BP                = 61; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_FILTER_MODE_HP                = 62; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE_OFF           = 63; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE                 = 64; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO                 = 72; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE               = 80; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_ONE             = 65; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_TWO             = 73; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_LSB_VOICE_THREE           = 81; // 7-bit value (14-bit total)

const byte MIDI_CONTROL_CHANGE_SET_VOLUME                           = 43; // 4-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_SID_TRANSFER_DEBUGGING        = 119; // 1-bit value
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME_LSB                   = 124; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_SET_GLIDE_TIME                       = 125; // 7-bit value (14-bit total)
const byte MIDI_CONTROL_CHANGE_TOGGLE_ALL_TEST_BITS                 = 126; // 1-bit value

const byte MIDI_CONTROL_CHANGE_TOGGLE_VOLUME_MODULATION_MODE        = 84; // 1-bit value
const byte MIDI_CONTROL_CHANGE_TOGGLE_PULSE_WIDTH_MODULATION_MODE   = 83; // 1-bit value

const byte MIDI_CONTROL_CHANGE_RPN_MSB                              = 101;
const byte MIDI_CONTROL_CHANGE_RPN_LSB                              = 100;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY                           = 6;
const byte MIDI_CONTROL_CHANGE_DATA_ENTRY_FINE                      = 38;
const byte MIDI_RPN_PITCH_BEND_SENSITIVITY                          = 0;
const byte MIDI_RPN_MASTER_FINE_TUNING                              = 1;
const byte MIDI_RPN_MASTER_COARSE_TUNING                            = 2;
const word MIDI_RPN_NULL                                            = 16383;

// below are CC numbers that are defined and we don't implement, but repurposing
// them in the future might have weird consequences, so we shouldn't
const byte MIDI_CONTROL_CHANGE_ALL_NOTES_OFF                        = 123;

const byte MIDI_CHANNEL = 0; // "channel 1" (zero-indexed)
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_PARAPHONIC           = 0;
const byte MIDI_PROGRAM_CHANGE_SET_GLOBAL_MODE_MONOPHONIC_UNISON    = 1;
const byte MIDI_PROGRAM_CHANGE_HARDWARE_RESET                       = 127;

#endif /* SRC_MIDI_CONSTANTS_H */
