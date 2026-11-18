#include "strokes_mcu.h"

/* Letter 'a' -- bowl then stem+tail. Copied verbatim from test/strokes.c
 * so the MCU build does not reach into the laptop-test folder. */
const stroke_pt_t STROKE_A[] = {
    /* bowl, counter-clockwise from the top right */
    {840, 320, 1}, {760, 240, 1}, {640, 200, 1}, {520, 200, 1},
    {400, 240, 1}, {280, 320, 1}, {200, 400, 1}, {160, 520, 1},
    {160, 640, 1}, {200, 760, 1}, {280, 840, 1}, {400, 840, 1},
    {520, 760, 1}, {640, 640, 1}, {760, 520, 1}, {840, 400, 1},
    /* stem, top to bottom, with a tail */
    {880, 280, 1}, {880, 440, 1}, {880, 600, 1}, {840, 760, 1},
    {840, 880, 1}, {880, 920, 1},
};

const uint16_t STROKE_A_N = (uint16_t)(sizeof(STROKE_A) / sizeof(STROKE_A[0]));
