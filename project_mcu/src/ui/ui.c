#include "ui.h"
#include "config.h"

/* Button state */
static int      raw_level;        /* last raw sample */
static uint32_t raw_since;        /* when raw_level last changed */
static int      stable;           /* debounced level */
static uint32_t press_start;      /* when the debounced press began */
static int      long_fired;       /* long event already sent for this press */

/* LED state */
static uint32_t pulse_until;
static int      pulse_active;
static int      error_on;

void ui_init(void)
{
    raw_level = 0;
    raw_since = 0;
    stable = 0;
    press_start = 0;
    long_fired = 0;
    pulse_active = 0;
    error_on = 0;
}

ui_event_t ui_button_update(int pressed, uint32_t now_ms)
{
    pressed = (pressed != 0);

    /* Debounce: a new level counts only after it has held for UI_DEBOUNCE_MS. */
    if (pressed != raw_level) {
        raw_level = pressed;
        raw_since = now_ms;
    }
    if (raw_level != stable && (uint32_t)(now_ms - raw_since) >= UI_DEBOUNCE_MS) {
        stable = raw_level;
        if (stable) {
            press_start = raw_since;     /* time the press really started */
            long_fired = 0;
        } else {
            uint32_t held = raw_since - press_start;
            if (!long_fired && held < UI_SHORT_MAX_MS) {
                return UI_EVENT_SHORT;
            }
            /* Held between short and long, or long already sent: no event. */
        }
    }

    /* Long press fires while still held, so the user gets feedback at 2 s. */
    if (stable && !long_fired && (uint32_t)(now_ms - press_start) >= UI_LONG_MS) {
        long_fired = 1;
        return UI_EVENT_LONG;
    }
    return UI_EVENT_NONE;
}

void ui_led_pulse(uint32_t now_ms)
{
    pulse_until = now_ms + UI_PULSE_MS;
    pulse_active = 1;
}

void ui_led_error(void)
{
    error_on = 1;
}

void ui_led_clear_error(void)
{
    error_on = 0;
}

int ui_led_level(uint32_t now_ms)
{
    if (error_on) {
        return 1;
    }
    /* Signed difference handles millis() wrapping after 49.7 days. */
    if (pulse_active && (int32_t)(pulse_until - now_ms) > 0) {
        return 1;
    }
    pulse_active = 0;
    return 0;
}