#include "stroke.h"
#include "config.h"

static stroke_pt_t    buf[STROKE_MAX_POINTS];
static uint16_t       len;          /* entries used, breaks included */
static stroke_state_t state;
static stroke_stats_t stats;
static uint32_t       lift_ms;      /* when the finger left the pad */

void stroke_reset(void)
{
    len = 0;
    state = STROKE_IDLE;
    stats.n_points = 0;
    stats.n_strokes = 0;
    stats.overflow = 0;
}

stroke_state_t stroke_state(void)
{
    return state;
}

void stroke_get(const stroke_pt_t **pts, uint16_t *n)
{
    *pts = buf;
    *n = len;
}

void stroke_get_stats(stroke_stats_t *st)
{
    *st = stats;
}

/* Append one entry. Returns 0 if the buffer is full. */
static int push(uint16_t x, uint16_t y, uint8_t pen_down)
{
    if (len >= STROKE_MAX_POINTS) {
        stats.overflow = 1;
        return 0;
    }
    buf[len].x = x;
    buf[len].y = y;
    buf[len].pen_down = pen_down;
    len++;
    return 1;
}

/* Add a pen-down point, skipping exact repeats while the finger is still. */
static void add_point(const touch_sample_t *s)
{
    if (len > 0 && buf[len - 1].pen_down &&
        buf[len - 1].x == s->x && buf[len - 1].y == s->y) {
        return;
    }
    if (push(s->x, s->y, 1)) {
        stats.n_points++;
    }
}

/* A new stroke inside the same character: break, then its first point.
 * Needs two free slots, so the break is never left dangling at the end. */
static void start_new_stroke(const touch_sample_t *s)
{
    if (len + 2 > STROKE_MAX_POINTS) {
        stats.overflow = 1;
        return;
    }
    push(0, 0, 0);                  /* STROKE_BREAK */
    stats.n_strokes++;
    add_point(s);
}

stroke_state_t stroke_feed(const touch_sample_t *s, uint32_t now_ms)
{
    switch (state) {
    case STROKE_IDLE:
        if (s->touching) {
            stats.n_strokes = 1;
            add_point(s);
            state = STROKE_CAPTURING;
        }
        break;

    case STROKE_CAPTURING:
        if (s->touching) {
            add_point(s);
        } else {
            lift_ms = now_ms;
            state = STROKE_PENDING;
        }
        break;

    case STROKE_PENDING: {
        uint32_t gap = now_ms - lift_ms;          /* wrap-safe */
        if (s->touching) {
            if (gap < STROKE_LIFT_DEBOUNCE_MS) {
                add_point(s);                     /* dropout: same stroke */
            } else {
                start_new_stroke(s);              /* real lift: next stroke */
            }
            state = STROKE_CAPTURING;
        } else if (gap >= STROKE_END_GAP_MS) {
            state = STROKE_READY;
        }
        break;
    }

    case STROKE_READY:
        break;                                    /* hold until stroke_reset() */
    }
    return state;
}