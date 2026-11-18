// STM32L432KC_RCC.h
// Clock configuration: 80 MHz SYSCLK from PLL (MSI source).
//
// Ported from E155 Lab 7 by Haspard Murphy. Unchanged except for renamed
// comments and the additional AHB/APB prescaler setup in configureClock().

#ifndef STM32L4_RCC_H
#define STM32L4_RCC_H

#include <stdint.h>
#include <stm32l432xx.h>

void configurePLL(void);
void configureClock(void);

#endif // STM32L4_RCC_H
