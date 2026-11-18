#ifndef GOLDEN_H
#define GOLDEN_H

#include <stdint.h>
#include "bitmap.h"
#include "classes.h"

/* Load weights and thresholds from BNN/mem. Returns 0 on success */
int golden_load(const char *mem_dir);

int golden_infer(const uint64_t in[BITMAP_WORDS], uint16_t scores[NUM_CLASSES]);

#endif // GOLDEN_H