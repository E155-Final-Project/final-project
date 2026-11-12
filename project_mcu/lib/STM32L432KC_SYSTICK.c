// STM32L432KC_SYSTICK.c
// SysTick millisecond tick.
//
// Written for the E155 final project by Haspard Murphy.
// SysTick is configured off the AHB/8 clock (10 MHz at SYSCLK = 80 MHz).
// Reload = 10 000 - 1 for a 1 kHz interrupt. See ARM Cortex-M4 Generic
// User Guide section 4.5 (SysTick).

#include <stm32l432xx.h>
#include "STM32L432KC_SYSTICK.h"

static volatile uint32_t s_millis = 0;

void systickInit(void) {
    // SYSCLK/8 = 80 MHz / 8 = 10 MHz.  Reload for 1 ms = 10 000 ticks.
    SysTick->CTRL = 0;                                  // disable while editing
    SysTick->LOAD = 10000u - 1u;                        // VERIFY: assumes 80 MHz
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
    // CLKSOURCE bit left 0 -> processor clock / 8
}

uint32_t millis(void) {
    return s_millis;
}

void delay_ms(uint32_t ms) {
    uint32_t start = s_millis;
    while ((uint32_t)(s_millis - start) < ms) {
        __asm__ volatile ("wfi");                       // sleep until next tick
    }
}

// Vector matches stm32l432xx_Vectors.s (SysTick_Handler).
void SysTick_Handler(void) {
    s_millis++;
}
