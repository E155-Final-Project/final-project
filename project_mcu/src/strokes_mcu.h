#ifndef STROKES_MCU_H
#define STROKES_MCU_H

#include <stdint.h>
#include "rasterize.h"

/* A single hardcoded stroke of the letter 'a', copied from test/strokes.c so
 * the hardware build does not depend on the laptop test folder. Keep in sync
 * by hand if test/strokes.c changes. */
extern const stroke_pt_t STROKE_A[];
extern const uint16_t    STROKE_A_N;

#endif
