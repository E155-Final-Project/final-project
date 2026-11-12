// STM32L432KC_RCC.c
// Clock init: 80 MHz SYSCLK from the PLL (MSI = 4 MHz, PLLN=80, PLLM=2,
// PLLR=2  ->  4 * 80 / 2 / 2 = 80 MHz).
//
// Ported from E155 Lab 7 by Haspard Murphy. Added explicit AHB/APB1/APB2
// prescaler writes so PCLK1 = PCLK2 = 80 MHz (lab 7 relied on reset defaults).

#include "STM32L432KC_RCC.h"
#include "STM32L432KC_FLASH.h"

void configurePLL(void) {
    // Output freq = (src_clk) * (N/M) / R
    // (4 MHz MSI) * (80/2) / 2  = 80 MHz

    RCC->CR &= ~RCC_CR_PLLON;                               // PLL off
    while ((RCC->CR & RCC_CR_PLLRDY) != 0);                 // wait until unlocked

    // PLL input mux = MSI; MSI defaults to 4 MHz at reset.
    RCC->PLLCFGR &= ~(RCC_PLLCFGR_PLLSRC | RCC_PLLCFGR_PLLM |
                      RCC_PLLCFGR_PLLN   | RCC_PLLCFGR_PLLR);
    RCC->PLLCFGR |= _VAL2FLD(RCC_PLLCFGR_PLLSRC, RCC_PLLCFGR_PLLSRC_MSI);
    RCC->PLLCFGR |= _VAL2FLD(RCC_PLLCFGR_PLLM, 0b001);      // M = 2
    RCC->PLLCFGR |= _VAL2FLD(RCC_PLLCFGR_PLLN, 80);         // N = 80
    RCC->PLLCFGR |= _VAL2FLD(RCC_PLLCFGR_PLLR, 0b00);       // R = 2
    RCC->PLLCFGR |= RCC_PLLCFGR_PLLREN;                     // route PLLCLK to SYSCLK

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0);                 // wait until locked
}

void configureClock(void) {
    // Flash wait states first -- 4 WS required at 80 MHz, VOS range 1
    // (RM0394 3.3.3 Table 9). Prefetch helps linear fetches.
    configureFlash();

    configurePLL();

    // AHB, APB1, APB2 prescalers = 1  (CFGR HPRE=0b0000, PPRE1=PPRE2=0b000)
    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);

    // Switch SYSCLK to PLL and wait until the mux reports PLL.
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL);

    SystemCoreClockUpdate();                                // CMSIS global
}
