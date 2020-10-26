#ifndef SRC_STDINOUT_H
#define SRC_STDINOUT_H

#include <stdio.h>
#include <Arduino.h>

// lets us use normal, sane print statements like `printf` and `fprintf`
// in arduino sketches.
// sets up a global file descriptor as a wrapper around actual calls to to
// Serial.write and Serial.read
//
// All you have to do is call `setup_stdin_stdout()` in `setup`.

void setup_stdin_stdout();

static FILE serial_stdinout;

// Function that printf and related will use to print
static int serial_putchar(char c, FILE* f) {
  if (c == '\n') serial_putchar('\r', f);
  return Serial.write(c) == 1? 0 : 1;
};

// Function that scanf and related will use to read
static int serial_getchar(FILE*) {
  while (Serial.available() <= 0);
  return Serial.read();
};

void setup_stdin_stdout() {
  fdev_setup_stream(&serial_stdinout, serial_putchar, serial_getchar, _FDEV_SETUP_RW);
  stdout = &serial_stdinout;
  stdin  = &serial_stdinout;
  stderr = &serial_stdinout;
};

#endif /* SRC_STDINOUT_H */
