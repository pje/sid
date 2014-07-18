#include <math.h>

const int ARDUINO_SID_CHIP_SELECT_PIN = 13;
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5;

const double CLOCK_SIGNAL_FACTOR = 0.0596;

const byte REGISTER_BANK_ADDRESS_VOICE_1 = 0;
const byte REGISTER_BANK_ADDRESS_VOICE_2 = 7;
const byte REGISTER_BANK_ADDRESS_VOICE_3 = 14;
const byte REGISTER_BANK_ADDRESS_FILTER  = 21;
const byte REGISTER_BANK_ADDRESS_MISC    = 25;

const byte REGISTER_BANK_OFFSET_VOICE_FREQUENCY_LO   = 0;
const byte REGISTER_BANK_OFFSET_VOICE_FREQUENCY_HI   = 1;
const byte REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_LO = 2;
const byte REGISTER_BANK_OFFSET_VOICE_PULSE_WIDTH_HI = 3;
const byte REGISTER_BANK_OFFSET_VOICE_CONTROL        = 4;
const byte REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD    = 5;
const byte REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR    = 6;

const byte SID_NOISE       = B10000000; // 128;
const byte SID_SQUARE      = B01000000; //  64;
const byte SID_RAMP        = B00100000; //  32;
const byte SID_TRIANGLE    = B00010000; //  16;
const byte SID_TEST        = B00001000; //   8;
const byte SID_RING        = B00000100; //  20; // requires triangle
const byte SID_SYNC        = B01000010; //  66; // requires OSC3 at freq > 0
const byte SID_3OFF        = B10000000; // 128;
const byte SID_FILT_HP     = B01000000; //  64;
const byte SID_FILT_BP     = B00100000; //  32;
const byte SID_FILT_LP     = B00010000; //  16;
const byte SID_FILT_OFF    = B00000000; //   0;
const byte SID_FILT_VOICE1 = B00000001; //   1;
const byte SID_FILT_VOICE2 = B00000010; //   2;
const byte SID_FILT_VOICE3 = B00000100; //   4;
const byte SID_FILT_EXT    = B00001000; //   8;

const byte SID_FL_FL     = 21;
const byte SID_FL_FH     = 22;
const byte SID_FL_RES_CT = 23;
const byte SID_FL_MD_VL  = 24;

// since we have to set all the bits in a control register byte at once,
// we must maintain a copy of the register's state so we don't clobber bits
byte voice_control_register_state_bytes[3] = {
  B00100000, // control register byte for Voice 1
  B00100000, // control register byte for Voice 2
  B00100000  // control register byte for Voice 3
};

byte voice_ad_register_state_bytes[3] = {
  B00000000, // attack/decay register byte for Voice 1
  B00000000, // attack/decay register byte for Voice 2
  B00000000  // attack/decay register byte for Voice 3
};

byte voice_sr_register_state_bytes[3] = {
  B00000000, // sustain/release register byte for Voice 1
  B00000000, // sustain/release register byte for Voice 2
  B00000000  // sustain/release register byte for Voice 3
};

byte filter_register_state_byte = B00000000;
byte mode_register_state_byte   = B00000000;

const byte MIDI_NOTE_ON        = B1001;
const byte MIDI_NOTE_OFF       = B1000;
const byte MIDI_PITCH_BEND     = B1110;
const byte MIDI_CONTROL_CHANGE = B1011;
const byte MIDI_PROGRAM_CHANGE = B1100;
const byte MIDI_TIMING_CLOCK   = B11111000;

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_TRIANGLE   = 12;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_RAMP       = 13;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_SQUARE     = 14;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_ONE_NOISE      = 15;
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_ONE            = 16;
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE                = 17;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_ONE              = 18;
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE         = 38;
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_ONE              = 39;
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE               = 40;
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE             = 41;
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE             = 42;

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_TRIANGLE   = 20;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_RAMP       = 21;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_SQUARE     = 22;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_TWO_NOISE      = 23;
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO            = 24;
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO                = 25;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_TWO              = 26;
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO         = 46;
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO              = 47;
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO               = 48;
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO             = 49;
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO             = 50;

const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_TRIANGLE = 28;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_RAMP     = 29;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_SQUARE   = 30;
const byte MIDI_CONTROL_CHANGE_SET_WAVEFORM_VOICE_THREE_NOISE    = 31;
const byte MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE          = 32;
const byte MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE              = 33;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE            = 34;
const byte MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE       = 54;
const byte MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE            = 55;
const byte MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE             = 56;
const byte MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE           = 57;
const byte MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE           = 58;

const byte MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY              = 36;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE              = 37;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_MODE_LP                = 60;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_MODE_BP                = 61;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_MODE_HP                = 62;
const byte MIDI_CONTROL_CHANGE_SET_FILTER_VOICE_THREE_OFF        = 63;
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_ONE              = 64;
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO              = 72;
const byte MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE            = 80;

const byte MIDI_CONTROL_CHANGE_SET_VOLUME                        = 43;

const byte MIDI_CHANNEL = 0; // "channel 1" (zero-indexed)

const byte PROGRAM_MODE_POLYPHONIC = 0;
const byte PROGRAM_MODE_MONOPHONIC = 1;

byte current_program = PROGRAM_MODE_POLYPHONIC;

const byte MAX_POLYPHONY = 3;
byte polyphony = 3;
byte current_notes[MAX_POLYPHONY];

int midi_pitch_bend_max_semitones = 5;
double current_pitchbend_amount = 0.0; // [-1.0 .. 1.0]

const int DEFAULT_PULSE_WIDTH_VALUE = 2048; // (2**12 - 1) / 2
const int DEFAULT_WAVEFORM_VALUE = SID_TRIANGLE;
const int DEFAULT_ATTACK_VALUE = 0;
const int DEFAULT_DECAY_VALUE = 14;
const int DEFAULT_SUSTAIN_VALUE = 0;
const int DEFAULT_RELEASE_VALUE = 0;

const double MIDI_NOTES_TO_FREQUENCIES[96] = { 16.351597831287414, 17.323914436054505, 18.354047994837977, 19.445436482630058, 20.601722307054366, 21.826764464562746, 23.12465141947715, 24.499714748859326, 25.956543598746574, 27.5, 29.13523509488062, 30.86770632850775, 32.70319566257483, 34.64782887210901, 36.70809598967594, 38.890872965260115, 41.20344461410875, 43.653528929125486, 46.2493028389543, 48.999429497718666, 51.91308719749314, 55.0, 58.27047018976124, 61.7354126570155, 65.40639132514966, 69.29565774421802, 73.41619197935188, 77.78174593052023, 82.4068892282175, 87.30705785825097, 92.4986056779086, 97.99885899543733, 103.82617439498628, 110.0, 116.54094037952248, 123.47082531403103, 130.8127826502993, 138.59131548843604, 146.8323839587038, 155.56349186104046, 164.81377845643496, 174.61411571650194, 184.9972113558172, 195.99771799087463, 207.65234878997256, 220.0, 233.08188075904496, 246.94165062806206, 261.6255653005986, 277.1826309768721, 293.6647679174076, 311.1269837220809, 329.6275569128699, 349.2282314330039, 369.9944227116344, 391.99543598174927, 415.3046975799451, 440.0, 466.1637615180899, 493.8833012561241, 523.2511306011972, 554.3652619537442, 587.3295358348151, 622.2539674441618, 659.2551138257398, 698.4564628660078, 739.9888454232688, 783.9908719634985, 830.6093951598903, 880.0, 932.3275230361799, 987.7666025122483, 1046.5022612023945, 1108.7305239074883, 1174.6590716696303, 1244.5079348883237, 1318.5102276514797, 1396.9129257320155, 1479.9776908465376, 1567.981743926997, 1661.2187903197805, 1760.0, 1864.6550460723597, 1975.533205024496, 2093.004522404789, 2217.4610478149766, 2349.31814333926, 2489.0158697766474, 2637.02045530296, 2793.825851464031, 2959.955381693075, 3135.9634878539946, 3322.437580639561, 3520.0, 3729.3100921447194, 3951.066410048992 };

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
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, LOW);     // enable write mode on the SID ("CS must be low for any transfer")
  delayMicroseconds(2);                               // 2 microseconds should be enough for a single clock pulse to get through
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);    // disable write mode on the SID ("A write can only occur if CS is low, Ø2 is high and r/w is low")
}

void sid_zero_all_registers() {
  for (byte i = 0; i < 25; i++) {
    sid_transfer(i, B00000000);
  }
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    voice_control_register_state_bytes[i] = 0;
    voice_ad_register_state_bytes[i] = 0;
    voice_sr_register_state_bytes[i] = 0;
  }
  mode_register_state_byte = 0;
  filter_register_state_byte = 0;
}

void sid_zero_voice_registers(int voice) {
  byte address = 0;
  for (int i = 0; i < 7; i++) {
    address = (voice * 7) + i;
    sid_transfer(address, 0);
  }
  voice_control_register_state_bytes[voice] = 0;
  voice_ad_register_state_bytes[voice] = 0;
  voice_sr_register_state_bytes[voice] = 0;
}

void sid_set_volume(byte level) {
  mode_register_state_byte &= B11110000;
  mode_register_state_byte |= (level & B00001111);
  sid_transfer(SID_FL_MD_VL, mode_register_state_byte);
}

void sid_set_waveform(int voice, byte waveform) {
  voice_control_register_state_bytes[voice] = (waveform | (voice_control_register_state_bytes[voice] & B00001111));
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = voice_control_register_state_bytes[voice];
  sid_transfer(address, data);
}

void sid_set_ring_mod(int voice, boolean on) {
  if (on) {
    // ring mod repurposes the output of the triangle oscillator
    voice_control_register_state_bytes[voice] |= B00010100; // set triangle and ring mod bits, leave others as-is
  } else {
    if (((voice_control_register_state_bytes[voice] >> 7) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_NOISE);
    } else if (((voice_control_register_state_bytes[voice] >> 6) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_SQUARE);
    } else if (((voice_control_register_state_bytes[voice] >> 5) & 0B00000001) == 1) {
      sid_set_waveform(voice, SID_RAMP);
    } else {
      sid_set_waveform(voice, SID_TRIANGLE);
    }
    voice_control_register_state_bytes[voice] &= B11111011; // zero ring mod
  }

  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = voice_control_register_state_bytes[voice];
  sid_transfer(address, data);
}

void sid_set_sync(int voice, boolean on) {
  if (on) {
    voice_control_register_state_bytes[voice] |= B00000010;
  } else {
    voice_control_register_state_bytes[voice] &= B11111101;
  }

  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = voice_control_register_state_bytes[voice];
  sid_transfer(address, data);
}

void sid_set_attack(int voice, byte attack) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD;
  byte data = voice_ad_register_state_bytes[voice] & B00001111;
  data |= (attack << 4);
  voice_ad_register_state_bytes[voice] = data;
  sid_transfer(address, data);
}

void sid_set_decay(int voice, byte decay) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD;
  byte data = voice_ad_register_state_bytes[voice] & B11110000;
  data |= (decay & B00001111);
  voice_ad_register_state_bytes[voice] = data;
  sid_transfer(address, data);
}

void sid_set_sustain(int voice, byte sustain) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR;
  byte data = voice_sr_register_state_bytes[voice] & B00001111;
  data |= (sustain << 4);
  voice_sr_register_state_bytes[voice] = data;
  sid_transfer(address, data);
}

void sid_set_release(int voice, byte release) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR;
  byte data = voice_sr_register_state_bytes[voice] & B11110000;
  data |= (release & B00001111);
  voice_sr_register_state_bytes[voice] = data;
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
  sid_transfer(SID_FL_FL, lo);
  sid_transfer(SID_FL_FH, hi);
}

void sid_set_filter_resonance(byte hertz) {
  filter_register_state_byte &= B00001111;
  filter_register_state_byte |= (hertz << 4);
  sid_transfer(SID_FL_RES_CT, filter_register_state_byte);
}

void sid_set_filter(byte voice, boolean on) {
  byte voice_filter_mask;
  if(voice == 0) {
    voice_filter_mask = SID_FILT_VOICE1;
  } else if(voice == 1) {
    voice_filter_mask = SID_FILT_VOICE2;
  } else if(voice == 2) {
    voice_filter_mask = SID_FILT_VOICE3;
  }
  if(on) {
    filter_register_state_byte |= voice_filter_mask;
  } else {
    filter_register_state_byte &= ~voice_filter_mask;
  }

  sid_transfer(SID_FL_RES_CT, filter_register_state_byte);
}

void sid_set_filter_mode(byte mode, boolean on) {
  mode_register_state_byte &= B00001111;

  if (on) {
    mode_register_state_byte |= mode;
  } else {
    mode_register_state_byte |= ~mode;
  }

  sid_transfer(SID_FL_MD_VL, mode_register_state_byte);
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
  if (state) {
    voice_control_register_state_bytes[voice] |= B00000001;
  } else {
    voice_control_register_state_bytes[voice] &= B11111110;
  }

  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = voice_control_register_state_bytes[voice];
  sid_transfer(address, data);
}

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

void nullify_current_notes() {
  for (int i = 0; i < MAX_POLYPHONY; i++) {
    current_notes[i] = NULL;
  }
}

void setup() {
  DDRF = B01110011; // initialize 5 PORTF pins as output (connected to A0-A4)
  DDRB = B11111111; // initialize 8 PORTB pins as output (connected to D0-D7)

  current_program = PROGRAM_MODE_POLYPHONIC;
  polyphony = 3;
  nullify_current_notes();

  start_clock();

  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  sid_zero_all_registers();
  for (int i = 0; i < 3; i++) {
    sid_set_pulse_width(i, DEFAULT_PULSE_WIDTH_VALUE);
    sid_set_waveform(i, DEFAULT_WAVEFORM_VALUE);
    sid_set_attack(i, DEFAULT_ATTACK_VALUE);
    sid_set_decay(i, DEFAULT_DECAY_VALUE);
    sid_set_sustain(i, DEFAULT_SUSTAIN_VALUE);
    sid_set_release(i, DEFAULT_RELEASE_VALUE);
  }
  sid_set_volume(15);

  Serial1.begin(31250, SERIAL_8N1);
  Serial.begin(31250);
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

void handle_message_voice_detune_change(byte voice, byte amount) {

}

void loop () {
  if (Serial1.available() > 0) {
    byte incomingByte = Serial1.read();
    byte opcode = incomingByte >> 4;
    byte data_byte_one = 0;
    byte data_byte_two = 0;
    int note_voice = 0;
    word pitchbend = 8192.0;
    byte controller_number = 0;
    byte controller_value = 0;
    double temp_double = 0.0;

    if(opcode >= B1000 && opcode <= B1110) { // Channel Voice/Mode Messages
      switch(opcode) {
        case MIDI_CONTROL_CHANGE :
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_one = Serial1.read();
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_two = Serial1.read();
          controller_number = data_byte_one;
          controller_value = data_byte_two;

          switch(controller_number) {
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
              handle_message_voice_ring_mod_change(0, controller_value == 127);
              break;
            case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_TWO:
              handle_message_voice_ring_mod_change(1, controller_value == 127);
              break;
            case MIDI_CONTROL_CHANGE_SET_RING_MOD_VOICE_THREE:
              handle_message_voice_ring_mod_change(2, controller_value == 127);
              break;

            case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_ONE:
              handle_message_voice_sync_change(0, controller_value == 127);
              break;
            case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_TWO:
              handle_message_voice_sync_change(1, controller_value == 127);
              break;
            case MIDI_CONTROL_CHANGE_SET_SYNC_VOICE_THREE:
              handle_message_voice_sync_change(2, controller_value == 127);
              break;

            case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_ONE:
              temp_double = (controller_value / 127.0) * 4095.0;
              handle_message_voice_pulse_width_change(0, (word)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_TWO:
              temp_double = (controller_value / 127.0) * 4095.0;
              handle_message_voice_pulse_width_change(1, (word)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_PULSE_WIDTH_VOICE_THREE:
              temp_double = (controller_value / 127.0) * 4095.0;
              handle_message_voice_pulse_width_change(2, (word)temp_double);
              break;

            case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_ONE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_attack_change(0, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_TWO:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_attack_change(1, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_ATTACK_VOICE_THREE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_attack_change(2, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_ONE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_decay_change(0, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_TWO:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_decay_change(1, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_DECAY_VOICE_THREE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_decay_change(2, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_ONE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_sustain_change(0, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_TWO:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_sustain_change(1, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_SUSTAIN_VOICE_THREE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_sustain_change(2, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_ONE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_release_change(0, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_TWO:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_release_change(1, (byte)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_RELEASE_VOICE_THREE:
              temp_double = (controller_value / 127.0) * 15.0;
              handle_message_voice_release_change(2, (byte)temp_double);
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
              handle_message_voice_detune_change(0, controller_value);
              break;
            case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_TWO:
              handle_message_voice_detune_change(1, controller_value);
              break;
            case MIDI_CONTROL_CHANGE_SET_DETUNE_VOICE_THREE:
              handle_message_voice_detune_change(2, controller_value);
              break;

            case MIDI_CONTROL_CHANGE_SET_FILTER_MODE_LP:
              if (controller_value == 127) {
                sid_set_filter_mode(SID_FILT_LP, true);
              } else if (controller_value == 0) {
                sid_set_filter_mode(SID_FILT_LP, false);
              }

              break;
            case MIDI_CONTROL_CHANGE_SET_FILTER_MODE_BP:
              if (controller_value == 127) {
                sid_set_filter_mode(SID_FILT_BP, true);
              } else if (controller_value == 0) {
                sid_set_filter_mode(SID_FILT_BP, false);
              }
              break;
            case MIDI_CONTROL_CHANGE_SET_FILTER_MODE_HP:
              if (controller_value == 127) {
                sid_set_filter_mode(SID_FILT_HP, true);
              } else if (controller_value == 0) {
                sid_set_filter_mode(SID_FILT_HP, false);
              }
              break;

            case MIDI_CONTROL_CHANGE_SET_FILTER_FREQUENCY:
              temp_double = (controller_value / 127.0) * 2047.0;
              sid_set_filter_frequency((word)temp_double);
              break;
            case MIDI_CONTROL_CHANGE_SET_FILTER_RESONANCE:
              temp_double = (controller_value / 127.0) * 15.0;
              sid_set_filter_resonance((byte)temp_double);
              break;

            case MIDI_CONTROL_CHANGE_SET_VOLUME:
              temp_double = (controller_value / 127.0) * 15;
              sid_set_volume((word)temp_double);
              break;
          }
          break;
        case MIDI_PROGRAM_CHANGE :
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_one = Serial1.read();
          switch(data_byte_one) {
            case PROGRAM_MODE_POLYPHONIC :
              polyphony = 3;
              for (int i = 1; i < MAX_POLYPHONY; i++) {
                voice_control_register_state_bytes[i] = voice_control_register_state_bytes[0];
                sid_transfer((i * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL, voice_control_register_state_bytes[i]);
              }
              break;
            case PROGRAM_MODE_MONOPHONIC :
              polyphony = 1;
              for (int i = 0; i < MAX_POLYPHONY; i++) {
                current_notes[i] = NULL;
                sid_zero_voice_registers(i);
                sid_set_attack(i, DEFAULT_ATTACK_VALUE);
                sid_set_decay(i, DEFAULT_DECAY_VALUE);
                sid_set_sustain(i, DEFAULT_SUSTAIN_VALUE);
                sid_set_release(i, DEFAULT_RELEASE_VALUE);
                sid_set_pulse_width(i, DEFAULT_PULSE_WIDTH_VALUE);
              }
              sid_set_waveform(0, DEFAULT_WAVEFORM_VALUE); // and why not turn on voice one by default
              break;
          }
          break;

        case MIDI_PITCH_BEND :
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_one = Serial1.read();
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_two = Serial1.read();

          pitchbend = data_byte_two;
          pitchbend = (pitchbend << 7);
          pitchbend |= data_byte_one;
          current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the middle pitchbend value (half of 2**14)
          for (int i = 0; i < polyphony; i++) {
            if (current_notes[i] != NULL) {
              note_voice = i;
              sid_set_voice_frequency(note_voice, (word)(MIDI_NOTES_TO_FREQUENCIES[current_notes[i]] * pow(2, (current_pitchbend_amount * midi_pitch_bend_max_semitones) / 12.0)));
            }
          }
          break;
        case MIDI_NOTE_ON :
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_one = Serial1.read();
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_two = Serial1.read();
          if (data_byte_one < 96) { // SID can't handle freqs > B7
            if (polyphony == 1) { // we're mono. for now all voices will have the same frequency.
              for (int i = 0; i < MAX_POLYPHONY; i++ ) {
                temp_double = (MIDI_NOTES_TO_FREQUENCIES[data_byte_one] * pow(2, (current_pitchbend_amount * midi_pitch_bend_max_semitones) / 12.0));
                sid_set_gate(i, false);
                sid_set_voice_frequency(i, (word)temp_double);
                sid_set_gate(i, true);
              }
            } else {  // we're poly, so every voice has to have its own frequency
              for (int i = 0; i < polyphony; i++) {
                if (current_notes[i] == NULL) {
                  current_notes[i] = data_byte_one;
                  note_voice = i;
                  break;
                }
                else {
                  if (i == (polyphony - 1)) {
                    current_notes[0] = current_notes[1];
                    current_notes[1] = current_notes[2];
                    current_notes[2] = data_byte_one;
                    note_voice = i;
                    break;
                  }
                }
              }

              temp_double = (MIDI_NOTES_TO_FREQUENCIES[data_byte_one] * pow(2, (current_pitchbend_amount * midi_pitch_bend_max_semitones) / 12.0));
              sid_set_gate(note_voice, false);
              sid_set_voice_frequency(note_voice, (word)temp_double);
              sid_set_gate(note_voice, true);
            }
          }
          break;
        case MIDI_NOTE_OFF :
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_one = Serial1.read();
          while (Serial1.available() <= 0) { delayMicroseconds(1); }
          data_byte_two = Serial1.read();
          // same for mono and poly
          for (int i = 0; i < MAX_POLYPHONY; i++) {
            if (current_notes[i] == data_byte_one) {
              current_notes[i] = NULL;
              note_voice = i;
              sid_set_gate(note_voice, false);
            }
          }
          break;
      }
    }
  }
}
