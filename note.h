#ifndef NOTE_H
#define NOTE_H

#include <stdbool.h>
#include "util.h"

struct note {
  byte number;
  // The midi note number.
  unsigned long on_time;
  // The time we got the midi "note on" message. 0 if none so far
  unsigned long off_time;
  // The time we got the midi "note off" message. 0 if none so far
  unsigned char voiced_by_oscillator;
  // The oscillator currently sounding the note.
  // -1 if no oscillator is voicing it (e.g. in a legato run, with 5 notes held
  // down, only the last one should be voiced. But we need to keep track of all
  // the "held" notes in case the user releases some of them.)
};

typedef struct note note;

#endif /* NOTE_H */

