#include <stdio.h>
#include <stdint.h>

#include "config.h"
#include "ui.h"

static int failures;

static void check(int ok, const char *what)
{
    printf("  %-48s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

/* Hold the button at `level` for `dur` ms in 1 ms steps.
 * Returns the first event seen, and its time in *at. */
static ui_event_t run(int level, uint32_t *t, uint32_t dur, uint32_t *at)
{
    ui_event_t first = UI_EVENT_NONE;
    for (uint32_t i = 0; i < dur; i++, (*t)++) {
        ui_event_t e = ui_button_update(level, *t);
        if (e != UI_EVENT_NONE && first == UI_EVENT_NONE) {
            first = e;
            if (at) *at = *t;
        }
    }
    return first;
}

/* Bouncy contact: alternate every 2 ms for `dur` ms. */
static ui_event_t bounce(uint32_t *t, uint32_t dur)
{
    ui_event_t first = UI_EVENT_NONE;
    for (uint32_t i = 0; i < dur; i++, (*t)++) {
        ui_event_t e = ui_button_update((int)((i / 2) % 2), *t);
        if (e != UI_EVENT_NONE && first == UI_EVENT_NONE) first = e;
    }
    return first;
}

int main(void)
{
    uint32_t t = 1000, at = 0;
    ui_event_t e;

    printf("button:\n");
    ui_init();
    run(0, &t, 100, NULL);
    e = run(1, &t, 200, NULL);
    check(e == UI_EVENT_NONE, "200 ms press: nothing while held");
    e = run(0, &t, 100, NULL);
    check(e == UI_EVENT_SHORT, "200 ms press: SHORT on release");

    ui_init();
    e = bounce(&t, 30);
    e = (e != UI_EVENT_NONE) ? e : run(1, &t, 150, NULL);
    check(e == UI_EVENT_NONE, "bouncy press: no event while held");
    e = bounce(&t, 30);
    e = (e != UI_EVENT_NONE) ? e : run(0, &t, 100, NULL);
    check(e == UI_EVENT_SHORT, "bouncy press: exactly one SHORT");
    e = run(0, &t, 500, NULL);
    check(e == UI_EVENT_NONE, "bouncy press: nothing afterwards");

    ui_init();
    e = run(1, &t, 10, NULL);
    e = (e != UI_EVENT_NONE) ? e : run(0, &t, 100, NULL);
    check(e == UI_EVENT_NONE, "10 ms glitch: ignored");

    ui_init();
    e = run(1, &t, 1000, NULL);
    e = (e != UI_EVENT_NONE) ? e : run(0, &t, 100, NULL);
    check(e == UI_EVENT_NONE, "1 s press (between short and long): nothing");

    ui_init();
    uint32_t start = t;
    e = run(1, &t, 2500, &at);
    check(e == UI_EVENT_LONG, "2.5 s hold: LONG while held");
    check(at - start == UI_LONG_MS, "LONG fires at exactly 2000 ms");
    e = run(0, &t, 100, NULL);
    check(e == UI_EVENT_NONE, "long press release: no SHORT");

    ui_init();
    t = 0xFFFFFF00u;                      /* millis() wrap after ~49.7 days */
    run(0, &t, 50, NULL);
    e = run(1, &t, 200, NULL);
    e = (e != UI_EVENT_NONE) ? e : run(0, &t, 100, NULL);
    check(e == UI_EVENT_SHORT, "short press across millis() wrap");

    printf("led:\n");
    ui_init();
    t = 5000;
    check(ui_led_level(t) == 0, "idle: off");
    ui_led_pulse(t);
    check(ui_led_level(t + 1) == 1, "pulse: on at +1 ms");
    check(ui_led_level(t + UI_PULSE_MS - 1) == 1, "pulse: on just before end");
    check(ui_led_level(t + UI_PULSE_MS) == 0, "pulse: off at end");
    ui_led_error();
    check(ui_led_level(t + 10000) == 1, "error: solid on");
    ui_led_clear_error();
    check(ui_led_level(t + 10001) == 0, "error cleared: off");

    printf("\ntest ui: %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}