const int ARDUINO_SPI_DATA_PIN  = 11;
const int ARDUINO_SPI_CLOCK_PIN = 3;
const int ARDUINO_SPI_LATCH_PIN = 13;
const int ARDUINO_SID_CHIP_SELECT_PIN = 2;
const int ARDUINO_SID_MASTER_CLOCK_PIN = 9;

// 1-bit* flags
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

// VOICE ONE
const byte SID_V1_FL     =  0;
const byte SID_V1_FH     =  1;
const byte SID_V1_PWL    =  2;
const byte SID_V1_PWH    =  3;
const byte SID_V1_CT     =  4;
const byte SID_V1_AD     =  5;
const byte SID_V1_SR     =  6;

// VOICE TWO
const byte SID_V2_FL     =  7;
const byte SID_V2_FH     =  8;
const byte SID_V2_PWL    =  9;
const byte SID_V2_PWH    = 10;
const byte SID_V2_CT     = 11;
const byte SID_V2_AD     = 12;
const byte SID_V2_SR     = 13;

// VOICE THREE
const byte SID_V3_FL     = 14;
const byte SID_V3_FH     = 15;
const byte SID_V3_PWL    = 16;
const byte SID_V3_PWH    = 17;
const byte SID_V3_CT     = 18;
const byte SID_V3_AD     = 19;
const byte SID_V3_SR     = 20;

// FILTER
const byte SID_FL_FL     = 21;
const byte SID_FL_FH     = 22;
const byte SID_FL_RES_CT = 23;
const byte SID_FL_MD_VL  = 24;

// READ-ONLY REGISTERS
const byte SID_POTX      = 25;
const byte SID_POTY      = 26;
const byte SID_OSC3_RND  = 27;
const byte SID_ENV3      = 28;

byte voice1_register = B00100000;
byte voice2_register = B00100000;
byte voice3_register = B00100000;
byte filter_register = B00000000;
byte mode_register   = B00000000;

#define DEBUG 0

void sid_transfer(byte sid_address, byte sid_data) {
  digitalWrite(ARDUINO_SPI_LATCH_PIN, LOW);
  shiftOut(ARDUINO_SPI_DATA_PIN, ARDUINO_SPI_CLOCK_PIN, LSBFIRST, sid_address);
  shiftOut(ARDUINO_SPI_DATA_PIN, ARDUINO_SPI_CLOCK_PIN, LSBFIRST, sid_data);

  digitalWrite(ARDUINO_SPI_LATCH_PIN, HIGH);

  #ifdef DEBUG
    Serial.print(sid_address, BIN);
    Serial.print(" ");
    Serial.print(sid_data,    BIN);
    Serial.print("\n");
  #endif

  // wait a small amount of time for the shift register latch data in
  delayMicroseconds(2);
  // enable write mode on the SID ("CS must be low for any transfer")
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, LOW);
  // 2 microseconds should be enough for a single clock pulse to get through
  delayMicroseconds(2);
  // disable write mode on the SID ("A read can only occur if CS is low, Ø2 is high and r/w is low")
  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);
}

void sid_zero_all_registers() {
  for (byte i = 0; i < 25; i++) {
    sid_transfer(i, B00000000);
  }
}

void sid_set_volume(byte level) {
  mode_register &= B11110000;
  mode_register |= (level & B00001111);
  sid_transfer(SID_FL_MD_VL, mode_register);
}

void sid_set_waveform_voice_one(byte waveform) {
  voice1_register &= B00000001; // sets the GATE bit to 1, initiating attack phase
  voice1_register |= waveform;
  sid_transfer(SID_V1_CT, voice1_register);
}

void sid_set_waveform_voice_two(byte waveform) {
  voice2_register &= B00000001;
  voice2_register |= waveform;
  sid_transfer(SID_V2_CT, voice2_register);
}

void sid_set_waveform_voice_three(byte waveform) {
  voice3_register &= B00000001;
  voice3_register |= waveform;
  sid_transfer(SID_V3_CT, voice3_register);
}

void sid_set_ad_envelope_voice_one(byte attack, byte decay) {
  sid_transfer(SID_V1_AD, (decay & B00001111) | (attack << 4));
}

void sid_set_ad_envelope_voice_two(byte attack, byte decay) {
  sid_transfer(SID_V2_AD, (decay & B00001111) | (attack << 4));
}

void sid_set_ad_envelope_voice_three(byte attack, byte decay) {
  sid_transfer(SID_V3_AD, (decay & B00001111) | (attack << 4));
}

void sid_set_sr_envelope_voice_one(byte sustain, byte release) {
  sid_transfer(SID_V1_SR, (release & B00001111) | (sustain << 4));
}

void sid_set_sr_envelope_voice_two(byte sustain, byte release) {
  sid_transfer(SID_V2_SR, (release & B00001111) | (sustain << 4));
}

void sid_set_sr_envelope_voice_three(byte sustain, byte release) {
  sid_transfer(SID_V3_SR, (release & B00001111) | (sustain << 4));
}

void sid_set_filter_mode(byte mode) {
  mode_register &= B00001111;
  mode_register |= mode;
  sid_transfer(SID_FL_MD_VL, mode_register);
}

void sid_set_voice_frequency(byte lowAddress, byte highAddress, word frequency) {
  byte low = lowByte(frequency);
  byte high = highByte(frequency);
  sid_transfer(lowAddress, low);
  sid_transfer(highAddress, high);
}

void sid_set_frequency_voice_one(word frequency) {
  sid_set_voice_frequency(SID_V1_FL, SID_V1_FH, frequency);
}

void sid_set_frequency_voice_two(word frequency) {
  sid_set_voice_frequency(SID_V2_FL, SID_V2_FH, frequency);
}

void sid_set_frequency_voice_three(word frequency) {
  sid_set_voice_frequency(SID_V3_FL, SID_V3_FH, frequency);
}

void sid_set_on_voice_one() {
  voice1_register |= B00000001;
  sid_transfer(SID_V1_CT, voice1_register);
}

void sid_set_off_voice_one() {
  voice1_register &= B11111110;
  sid_transfer(SID_V1_CT, voice1_register);
}

void sid_set_on_voice_two() {
  voice2_register |= B00000001;
  sid_transfer(SID_V2_CT, voice2_register);
}

void sid_set_on_voice_three() {
  voice3_register |= B00000001;
  sid_transfer(SID_V3_CT, voice3_register);
}

void sid_set_off_voice_three() {
  voice3_register &= B11111110;
  sid_transfer(SID_V3_CT, voice3_register);
}

void start_clock() {
  pinMode(ARDUINO_SID_MASTER_CLOCK_PIN, OUTPUT);

  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 7;
  TCCR1A |= (1 << COM1A0);
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS10);
}

void setup() {
  #ifdef DEBUG
    Serial.begin(9600);
  #endif

  pinMode(ARDUINO_SPI_LATCH_PIN, OUTPUT);
  pinMode(ARDUINO_SPI_CLOCK_PIN, OUTPUT);
  pinMode(ARDUINO_SPI_DATA_PIN, OUTPUT);
  pinMode(ARDUINO_SID_CHIP_SELECT_PIN, OUTPUT);

  digitalWrite(ARDUINO_SID_CHIP_SELECT_PIN, HIGH);
  digitalWrite(ARDUINO_SPI_LATCH_PIN, LOW);

  start_clock(); // SID requires a 1MHz clock pulse

  sid_zero_all_registers();
  sid_set_volume(15);
  sid_set_waveform_voice_one(SID_RAMP);
  sid_set_ad_envelope_voice_one(0, 0);
  sid_set_sr_envelope_voice_one(15, 0);
  sid_set_frequency_voice_one(1600);
  sid_set_on_voice_one();
}

word i = 274;

void loop() {
  sid_set_frequency_voice_one(i);

  i++;
  if (i >= 65535){
    i = 274;
  }
}
