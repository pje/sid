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
const byte SID_RING        = B00010100; //  20; // requires triangle
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
  B00100000, // control register byte for Voice 3
};

byte filter_register_state_byte = B00000000;
byte mode_register_state_byte   = B00000000;

const byte MIDI_NOTE_ON      = B1001;
const byte MIDI_NOTE_OFF     = B1000;
const byte MIDI_PITCH_BEND   = B1110;
const byte MIDI_TIMING_CLOCK = B11111000;

const byte MIDI_CHANNEL = 0; // "channel 1" (zero-indexed)

const byte MAX_POLYPHONY = 3;
byte polyphony = 3;
byte current_notes[MAX_POLYPHONY];

int midi_pitch_bend_max_semitones = 5;
double current_pitchbend_amount = 0.0; // [-1.0 .. 1.0]

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
}

void sid_set_volume(byte level) {
  mode_register_state_byte &= B11110000;
  mode_register_state_byte |= (level & B00001111);
  sid_transfer(SID_FL_MD_VL, mode_register_state_byte);
}

void sid_set_waveform(int voice, byte waveform) {
  voice_control_register_state_bytes[voice] &= B00000001;
  voice_control_register_state_bytes[voice] |= waveform;
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_CONTROL;
  byte data = voice_control_register_state_bytes[voice];
  sid_transfer(address, data);
}

void sid_set_ad_envelope(int voice, byte attack, byte decay) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_AD;
  byte data = (decay & B00001111) | (attack << 4);
  sid_transfer(address, data);
}

void sid_set_sr_envelope(int voice, byte sustain, byte release) {
  byte address = (voice * 7) + REGISTER_BANK_OFFSET_VOICE_ENVELOPE_SR;
  byte data = (release & B00001111) | (sustain << 4);
  sid_transfer(address, data);
}

void sid_set_filter_mode(byte mode) {
  mode_register_state_byte &= B00001111;
  mode_register_state_byte |= mode;
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

  nullify_current_notes();

  start_clock();

  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  sid_zero_all_registers();
  for (int i = 0; i < 3; i++) {
    sid_set_waveform(i, SID_TRIANGLE);
    sid_set_ad_envelope(i, 0, 15);
    sid_set_sr_envelope(i, 0, 0);
  }
  sid_set_volume(15);

  Serial1.begin(31250, SERIAL_8N1);
  // Serial.begin(31250);
}

void loop () {
  if (Serial1.available() > 0) {
    byte incomingByte = Serial1.read();
    byte opcode = incomingByte >> 4;
    byte note_number = 0;
    byte note_velocity = 0;
    byte current_note_0_temp = 0;
    byte current_note_1_temp = 0;
    byte current_note_2_temp = 0;
    int note_voice = 0;
    byte pitchbend_lsb = 0;
    byte pitchbend_msb = 0;
    word pitchbend = 8192.0;

    if(incomingByte != MIDI_TIMING_CLOCK) {
      switch(opcode) {
        case MIDI_PITCH_BEND :
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          pitchbend_lsb = Serial1.read();
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          pitchbend_msb = Serial1.read();

          pitchbend = pitchbend_msb;
          pitchbend = (pitchbend << 7);
          pitchbend |= pitchbend_lsb;
          current_pitchbend_amount = ((pitchbend / 8192.0) - 1); // 8192 is the middle pitchbend value (half of 2**14)
          for (int i = 0; i < polyphony; i++) {
            if (current_notes[i] != NULL) {
              note_voice = i;
              note_number = current_notes[i];
              sid_set_voice_frequency(note_voice, (word)(MIDI_NOTES_TO_FREQUENCIES[note_number] * pow(2, (current_pitchbend_amount * midi_pitch_bend_max_semitones) / 12.0)));
            }
          }
          break;
        case MIDI_NOTE_ON :
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          note_number = Serial1.read();
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          note_velocity = Serial1.read();
          if (note_number < 96) { // SID can't handle freqs > B7

            for (int i = 0; i < polyphony; i++) {
              if (current_notes[i] == NULL) {
                current_notes[i] = note_number;
                note_voice = i;
                break;
              }
              else {
                if (i == (polyphony - 1)) {
                  current_note_0_temp = current_notes[0];
                  current_note_1_temp = current_notes[1];
                  current_note_2_temp = current_notes[2];
                  current_notes[0] = current_note_1_temp;
                  current_notes[1] = current_note_2_temp;
                  current_notes[2] = note_number;
                  note_voice = i;
                  break;
                }
              }
            }
            sid_set_gate(note_voice, false);
            // initial_hertz * (2 ** (half_steps / 12.0))
            sid_set_voice_frequency(note_voice, (word)(MIDI_NOTES_TO_FREQUENCIES[note_number] * pow(2, (current_pitchbend_amount * midi_pitch_bend_max_semitones) / 12.0)));
            sid_set_gate(note_voice, true);
          }
          break;
        case MIDI_NOTE_OFF :
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          note_number = Serial1.read();
          while (Serial1.available() <= 0) {
            delayMicroseconds(1);
          }
          note_velocity = Serial1.read();
          for (int i = 0; i < polyphony; i++) {
            if (current_notes[i] == note_number) {
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
