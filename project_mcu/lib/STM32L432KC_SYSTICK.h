// STM32L432KC_SYSTICK.h
// SysTick-based millisecond time base.
//
// Written for the E155 final project by Haspard Murphy.
// Call systickInit() AFTER configureClock() so SystemCoreClock is 80 MHz.

#ifndef STM32L4_SYSTICK_H
#define STM32L4_SYSTICK_H

#include <stdint.h>

/* Program SysTick for a 1 kHz interrupt from SYSCLK/8. */
void systickInit(void);

/* Monotonic milliseconds since systickInit(). Overflows after ~49.7 days. */
uint32_t millis(void);

/* Busy-wait. Uses millis() so it is resistant to preemption. */
void delay_ms(uint32_t ms);

#endif // STM32L4_SYSTICK_H
