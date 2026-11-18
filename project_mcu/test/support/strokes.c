#include "strokes.h"
#define NPTS(a) ((uint16_t)(sizeof(a) / sizeof((a)[0])))

static const stroke_pt_t STROKE_A[] = {
    /* bowl, counter-clockwise from the top right */
    {840, 320, 1}, {760, 240, 1}, {640, 200, 1}, {520, 200, 1},
    {400, 240, 1}, {280, 320, 1}, {200, 400, 1}, {160, 520, 1},
    {160, 640, 1}, {200, 760, 1}, {280, 840, 1}, {400, 840, 1},
    {520, 760, 1}, {640, 640, 1}, {760, 520, 1}, {840, 400, 1},
    /* stem, top to bottom, with a tail */
    {880, 280, 1}, {880, 440, 1}, {880, 600, 1}, {840, 760, 1},
    {840, 880, 1}, {880, 920, 1},
};

static const stroke_pt_t STROKE_T[] = {
    {200, 240, 1}, {500, 240, 1}, {880, 240, 1},     /* top bar */
    STROKE_BREAK,
    {540, 240, 1}, {540, 560, 1}, {540, 920, 1},     /* stem */
};

const test_stroke_t TEST_STROKES[] = {
    {"a", 36, STROKE_A, NPTS(STROKE_A)},
    {"T", 29, STROKE_T, NPTS(STROKE_T)},
};

const int N_TEST_STROKES = (int)(sizeof(TEST_STROKES) / sizeof(TEST_STROKES[0]));

