// STM32L432KC.h
// Umbrella header: pulls in CMSIS device header and every peripheral driver.
//
// Ported from E155 Lab 7 (HMC, Prof. Harris / MCH template) by Haspard Murphy
// for the E155 final project. Added SysTick, EXTI, I2C; removed TIM (unused).

#ifndef STM32L4_H
#define STM32L4_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stm32l432xx.h>

// Peripheral drivers
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_RCC.h"
#include "STM32L432KC_FLASH.h"
#include "STM32L432KC_USART.h"
#include "STM32L432KC_SPI.h"
#include "STM32L432KC_SYSTICK.h"
#include "STM32L432KC_EXTI.h"
#include "STM32L432KC_I2C.h"

// Global clock defines (RM0394 6.2)
#define HSI_FREQ    16000000u   // HSI16 clock is 16 MHz
#define MSI_FREQ     4000000u   // MSI clock default is 4 MHz
#define SYSCLK_FREQ 80000000u   // PLL target in configureClock()
#define PCLK1_FREQ  80000000u   // APB1 = SYSCLK / 1 after configureClock()
#define PCLK2_FREQ  80000000u   // APB2 = SYSCLK / 1 after configureClock()

#endif // STM32L4_H
