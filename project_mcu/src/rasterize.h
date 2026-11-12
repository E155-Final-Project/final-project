#ifndef RASTERIZE_H
#define RASTERIZE_H

#include <stdint.h>
#include "bitmap.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t pen_down;
} stroke_pt_t;

#define STROKE_BREAK {0,0,0}

typedef struct {
    uint16_t n_points;
    uint16_t n_strokes;
    uint16_t raw_w;
    uint16_t raw_h;
    uint8_t box_rows;
    uint8_t box_cols;
    uint16_t ink;
} rast_stats_t;

int rasterize(const stroke_pt_t * pts, uint16_t n, uint64_t out[BITMAP_WORDS], rast_stats_t * st);

#endif