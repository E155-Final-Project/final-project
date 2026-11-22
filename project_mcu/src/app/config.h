#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

/* UI */
#define UI_DEBOUNCE_MS  20      /* level must be stable for this time */
#define UI_SHORT_MAX_MS 500     /* release before this */
#define UI_LONG_MS      2000    /* held for this long for FPGA reset */
#define UI_PULSE_MS     150     /* LED flash after classificaiton */

/* Trill Square */

/* Stroke Capture */
#define STROKE_MAX_POINTS       1024
#define STROKE_END_GAP_MS       500
#define STROKE_LIFT_DEBOUNCE_MS 30 

/* Raw coordinate range */
#define TRILL_X_MAX 1023
#define TRILL_Y_MAX 1023

/* Live view */
#define LIVE_TOP 2
#define LIVE_LEFT 3
#define LIVE_GAP 6

/* Rasterizer */

/* Sub-pixel resolution */
#define RAST_SUB        8

/* Longest side of the final in*/
#define RAST_TARGET_PX    22

/* Ink radius */
#define RAST_INK        12

/* Matches training */
#define RAST_THRESHOLD  128

/* Touchpad orientation */
#define RAST_SWAPXY     0
#define RAST_FLIP_X     0
#define RAST_fLIP_Y     0

_Static_assert(RAST_TARGET_PX * RAST_SUB > 2 * RAST_INK, "brush is wider than the character");

#endif
