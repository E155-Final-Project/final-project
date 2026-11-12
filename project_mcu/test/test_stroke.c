#include <stdio.h>
#include <stdint.h>

#include "bitmap.h"
#include "config.h"
#include "rasterize.h"
#include "stroke.h"
#include "strokes.h"

/* Test 7: the stroke-capture FSM, driven by a fake 200 Hz sample feed. */

#define TICK_MS 5                  /* 200 Hz, like the Trill polling */

static int failures;
static uint32_t now;

static void check(int ok, const char *what)
{
    printf("  %-56s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

static stroke_state_t touch(uint16_t x, uint16_t y)
{
    touch_sample_t s = {x, y, 1};
    stroke_state_t st = stroke_feed(&s, now);
    now += TICK_MS;
    return st;
}

/* No touch for `ms`. Returns the time READY first appeared, or 0 if it didn't. */
static uint32_t lift(uint32_t ms, uint32_t lift_start)
{
    touch_sample_t s = {0, 0, 0};
    uint32_t ready_at = 0;
    for (uint32_t i = 0; i < ms; i++) {   /* count steps: `now` may wrap past 0 */
        if (stroke_feed(&s, now) == STROKE_READY && ready_at == 0) {
            ready_at = now - lift_start;
        }
        now += 1;                  /* 1 ms steps here, to time the gap exactly */
    }
    return ready_at;
}

/* Feed a test stroke the way the Trill would: pen_down points as touches,
 * a STROKE_BREAK as a 200 ms lift. */
static void feed_test_stroke(const test_stroke_t *t)
{
    for (uint16_t i = 0; i < t->n; i++) {
        if (t->pts[i].pen_down) {
            touch(t->pts[i].x, t->pts[i].y);
        } else {
            lift(200, now);
        }
    }
}

static int same_bitmap(const uint64_t a[BITMAP_WORDS], const uint64_t b[BITMAP_WORDS])
{
    for (int k = 0; k < BITMAP_WORDS; k++) if (a[k] != b[k]) return 0;
    return 1;
}

int main(void)
{
    const stroke_pt_t *pts;
    uint16_t n;
    stroke_stats_t st;

    printf("single stroke:\n");
    now = 1000;
    stroke_reset();
    check(stroke_state() == STROKE_IDLE, "starts IDLE");
    lift(100, now);
    check(stroke_state() == STROKE_IDLE, "no touch: stays IDLE");
    touch(10, 10); touch(20, 20); touch(30, 30);
    check(stroke_state() == STROKE_CAPTURING, "touch: CAPTURING");
    uint32_t t_lift = now;
    uint32_t ready = lift(600, t_lift);
    check(ready == STROKE_END_GAP_MS, "READY exactly 500 ms after the lift");
    stroke_get(&pts, &n);
    stroke_get_stats(&st);
    check(n == 3 && st.n_points == 3 && st.n_strokes == 1, "3 points, 1 stroke, no breaks");
    touch(99, 99);
    stroke_get(&pts, &n);
    check(stroke_state() == STROKE_READY && n == 3, "READY ignores new touches until reset");

    printf("lifts inside a character:\n");
    stroke_reset();
    touch(10, 10); touch(20, 10);
    lift(10, now);                                 /* 10 ms dropout */
    touch(30, 10);
    stroke_get(&pts, &n);
    stroke_get_stats(&st);
    check(st.n_strokes == 1 && n == 3, "10 ms dropout: same stroke, no break");
    lift(499, now);
    check(stroke_state() == STROKE_PENDING, "499 ms lift: still PENDING");
    touch(30, 50); touch(30, 60);
    stroke_get(&pts, &n);
    stroke_get_stats(&st);
    check(st.n_strokes == 2 && n == 6 && pts[3].pen_down == 0,
          "touch after 499 ms: second stroke, break stored");
    t_lift = now;
    check(lift(600, t_lift) == STROKE_END_GAP_MS, "READY 500 ms after the final lift");
    check(pts[n - 1].pen_down == 1, "buffer never ends in a break");

    printf("repeats and clear:\n");
    stroke_reset();
    for (int i = 0; i < 50; i++) touch(40, 40);    /* finger held still */
    stroke_get(&pts, &n);
    check(n == 1, "50 identical samples stored once");
    stroke_reset();
    stroke_get(&pts, &n);
    check(stroke_state() == STROKE_IDLE && n == 0, "clear from CAPTURING: IDLE, empty");
    touch(1, 1); lift(100, now);
    stroke_reset();
    check(stroke_state() == STROKE_IDLE, "clear from PENDING: IDLE");
    touch(1, 1); lift(600, now);
    stroke_reset();
    check(stroke_state() == STROKE_IDLE, "clear from READY: IDLE");

    printf("buffer full:\n");
    stroke_reset();
    for (int i = 0; i < STROKE_MAX_POINTS + 200; i++) {
        touch((uint16_t)i, (uint16_t)(i / 2));     /* every sample distinct */
    }
    stroke_get(&pts, &n);
    stroke_get_stats(&st);
    check(n == STROKE_MAX_POINTS && st.overflow, "stops at STROKE_MAX_POINTS, flags overflow");
    lift(600, now);
    check(stroke_state() == STROKE_READY, "still reaches READY when full");

    printf("millis() wrap:\n");
    stroke_reset();
    now = 0xFFFFFF00u;                             /* about 256 ms before the wrap */
    touch(5, 5); touch(6, 6);
    t_lift = now;
    check(lift(600, t_lift) == STROKE_END_GAP_MS, "500 ms gap measured across the wrap");

    printf("end to end with the rasterizer:\n");
    for (int i = 0; i < N_TEST_STROKES; i++) {
        const test_stroke_t *t = &TEST_STROKES[i];
        uint64_t via_fsm[BITMAP_WORDS], direct[BITMAP_WORDS];
        now = 5000;
        stroke_reset();
        feed_test_stroke(t);
        lift(600, now);
        stroke_get(&pts, &n);
        rasterize(pts, n, via_fsm, NULL);
        rasterize(t->pts, t->n, direct, NULL);
        char what[64];
        snprintf(what, sizeof what, "'%s' via the FSM rasterizes identically", t->name);
        check(stroke_state() == STROKE_READY && same_bitmap(via_fsm, direct), what);
    }

    printf("\ntest 7 (stroke capture): %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}