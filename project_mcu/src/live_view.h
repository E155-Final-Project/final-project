#ifndef LIVE_VIEW_H
#define LIVE_VIEW_H

#include <stdint.h>
#include "bitmap.h"
#include "bitmap_print.h"
#include "rasterize.h"

void live_begin(char_out_function out);

void live_point(const stroke_pt_t *p);

void live_show_result(const uint64_t bm[BITMAP_WORDS], char cls);

void live_end(void);

#endif // LIVE_VIEW_H