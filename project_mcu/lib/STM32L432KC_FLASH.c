// STM32L432KC_FLASH.c
// Flash wait-state configuration.
//
// Ported from E155 Lab 7 by Haspard Murphy. At 80 MHz SYSCLK with
// VOS Range 1 (default), RM0394 Table 9 requires 4 wait states.

#include "STM32L432KC_FLASH.h"

void configureFlash(void) {
    // Set 4 WS and enable prefetch.
    FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;
    FLASH->ACR |= FLASH_ACR_PRFTEN;

    // Spin until the latency we just wrote is observable.
    while ((FLASH->ACR & FLASH_ACR_LATENCY) != FLASH_ACR_LATENCY_4WS);
}
