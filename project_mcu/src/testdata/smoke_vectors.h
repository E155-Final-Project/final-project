#ifndef SMOKE_VECTORS_H_INCLUDED
#define SMOKE_VECTORS_H_INCLUDED

#include <stdint.h>
#include "bitmap.h"

#define SMOKE_N 16

extern const uint64_t SMOKE_IMAGES[SMOKE_N][BITMAP_WORDS];
extern const uint8_t  SMOKE_EXPECTED[SMOKE_N];

#endif