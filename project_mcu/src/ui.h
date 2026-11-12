#ifndef UI_H
#define UI_H

#include <stdint.h>

/* Section G: clear button and status LED, as pure logic.
 * The caller supplies the time and the raw button level and drives the LED pin,
 * so this file has no hardware dependencies and is tested on the laptop. */

typedef enum {
    UI_EVENT_NONE = 0,
    UI_EVENT_SHORT,     /* press released before UI_SHORT_MAX_MS: clear */
    UI_EVENT_LONG,      /* held for UI_LONG_MS: FPGA reset (fires while held) */
} ui_event_t;

void ui_init(void);

/* Call every 1-5 ms. pressed = 1 while the button is down (after inverting the
 * active-low pin). Returns at most one event per press. */
ui_event_t ui_button_update(int pressed, uint32_t now_ms);

void ui_led_pulse(uint32_t now_ms);   /* short flash: a character was classified */
void ui_led_error(void);              /* solid on until cleared */
void ui_led_clear_error(void);

/* LED level to drive right now: 1 = on. Call as often as ui_button_update. */
int ui_led_level(uint32_t now_ms);

#endif