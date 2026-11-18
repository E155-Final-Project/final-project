#ifndef STROKE_H
#define STROKE_H

#include <stdint.h>
#include "rasterize.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint32_t touching;  /* milliseconds since stroke start */
} touch_sample_t;

typedef enum {
    STROKE_IDLE = 0,
    STROKE_CAPTURING,
    STROKE_PENDING,
    STROKE_READY,
} stroke_state_t;

typedef struct {
    uint16_t n_points;
    uint16_t n_strokes;
    uint8_t overflow;
} stroke_stats_t;

void stroke_reset(void);
stroke_state_t stroke_feed(const touch_sample_t *s, uint32_t now_ms);
stroke_state_t stroke_state(void);

void stroke_get(const stroke_pt_t **pts, uint16_t *n);
void stroke_get_stats(stroke_stats_t *st);

#endif