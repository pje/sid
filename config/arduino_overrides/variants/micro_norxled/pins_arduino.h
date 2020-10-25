#include "../micro/pins_arduino.h"

/* pje_begin_monkeypatch */

#undef RXLED0
#undef RXLED1

#define RXLED0 ((void)0)
#define RXLED1 ((void)0)

/* pje_monkeypatch_end */
