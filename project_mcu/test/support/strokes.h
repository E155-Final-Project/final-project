#ifndef STROKES_H
#define STROKES_H

#include <stdint.h>
#include "rasterize.h"

typedef struct {
    const char          *name;
    uint8_t             expected_class;
    const stroke_pt_t   *pts;
    uint16_t            n;
} test_stroke_t;

extern const test_stroke_t TEST_STROKES[];
extern const int            N_TEST_STROKES;

#endif