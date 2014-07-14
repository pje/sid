const int ARDUINO_SID_CHIP_SELECT_PIN = 13;
const int ARDUINO_SID_MASTER_CLOCK_PIN = 5;

const double TWELFTH_ROOT_OF_TWO = 1.0594630943592953;
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

void sid_play_monophonic(word frequency) {
  sid_set_waveform(0, SID_TRIANGLE);
  sid_set_ad_envelope(0, 0, 6);
  sid_set_sr_envelope(0, 15, 0);
  sid_set_voice_frequency(0, frequency);
  sid_set_gate(0, true);
  delay(100);
  sid_set_gate(0, false);
  delay(200);
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

void setup() {
  DDRF = B01110011; // initialize 5 PORTF pins as output (connected to A0-A4)
  DDRB = B11111111; // initialize 8 PORTB pins as output (connected to D0-D7)

  start_clock();

  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);

  sid_zero_all_registers();
  sid_set_volume(15);
}

void loop () {
  double a4  = 440.00;
  double cs4 = 554.37;
  double e5  = 659.25;
  double gs5 = 830.61;
  double a5  = 880.00;

  while (true) {
    sid_play_monophonic(a4);
    sid_play_monophonic(cs4);
    sid_play_monophonic(e5);
    sid_play_monophonic(gs5);
    sid_play_monophonic(a5);
    sid_play_monophonic(gs5);
    sid_play_monophonic(e5);
    sid_play_monophonic(cs4);
  }
}
