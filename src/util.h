#ifndef SRC_UTIL_H
#define SRC_UTIL_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef unsigned char byte;
typedef unsigned int word;

#ifndef constrain
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#endif /* constrain */

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

byte lowNibble(byte b) { return(b & 0B00001111); }
byte highNibble(byte b) { return((b & 0B11110000) >> 4); }

void print_byte_in_binary(byte b) {
  static char str[9];
  str[0] = '\0';

  for (int j = 128; j > 0; j >>= 1) {
    strcat(str, ((b & j) == j) ? "1" : "0");
  }

  printf("%s", str);
}

// need this because Arduino's default `sprintf` is broken with float input :(
void float_as_padded_string(char *str, float f, signed char mantissa_chars, signed char decimal_chars, char padding) {
  mantissa_chars = constrain(mantissa_chars, 0, 10);
  decimal_chars = constrain(decimal_chars, 0, 10);
  unsigned int len = mantissa_chars + decimal_chars + 1;

  #ifdef ARDUINO
    dtostrf(f, len, decimal_chars, str);
  #else
    sprintf(str, "%04f", f);
  #endif

  char format[20];
  sprintf(format, "%%%ds", len);

  sprintf(str, format, str);

  for(unsigned int i = 0; i < len; i++) {
    if (' ' == str[i]) {
      str[i] = padding;
    }
  }
}

// y(t) = A * sin(2πft + φ)
//
// returns a float between `-amplitude` and `amplitude`
float sine_waveform(float frequency, float seconds, float amplitude, float phase) {
  float value = amplitude * sin((2 * 3.141592 * frequency * seconds) + phase);
  return value;
}

// returns [0..1]
float linear_envelope(float a, float d, float s, float r, float seconds, float seconds_since_release_start) {
  if (seconds <= a) { // attack
    return(seconds / a);
  } else if (a < seconds && seconds <= (a + d)) { // decay
    return((((s - 1) * (seconds - a)) / d) + 1);
  } else if (seconds_since_release_start > 0) { // release
    return((((-1 * s) * seconds_since_release_start) / r) + s);
  } else { // sustain
    return(s);
  }
}

static const float note_frequency_lookup_table[] = {
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

// there's apparently not widespread agreement on which frequency midi notes
// represent. What we have below is "scientific pitch notation". Ableton, maxmsp
// and garageband use C3, which is shifted an octave lower.
// https://en.wikipedia.org/wiki/Scientific_pitch_notation#See_also
const float TWELFTH_ROOT_OF_TWO = 1.0594630943592953;
const unsigned int base_number = 57;
const unsigned int base_freq = 440;

// float note_number_to_frequency(byte note) {
//   int half_steps_from_base = note - base_number;

//   return(base_freq * (pow(TWELFTH_ROOT_OF_TWO, half_steps_from_base)));
// }

float note_number_to_frequency(byte note) {
  return note_frequency_lookup_table[note];
}

#endif /* SRC_UTIL_H */
