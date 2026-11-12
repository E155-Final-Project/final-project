// STM32L432KC_EXTI.h
// External interrupts for the FPGA "done" line and the "clear" button.
//
// Written for the E155 final project by Haspard Murphy. CMSIS style ported
// from the lab-5 EXTI driver (which used a hand-rolled register header).
//
// Scope is deliberately narrow: this file handles exactly the two EXTI lines
// used by the project (EXTI0 rising for PIN_FPGA_DONE, EXTI1 falling for
// PIN_BTN_CLEAR). Handlers set volatile flags only; main loop clears them.

#ifndef STM32L4_EXTI_H
#define STM32L4_EXTI_H

#include <stdint.h>

/* Configure PIN_FPGA_DONE and PIN_BTN_CLEAR and enable EXTI0 + EXTI1 in NVIC.
 * Must be called AFTER gpioEnable() has turned on the ports containing those
 * pins (configureClock() + gpioEnable(GPIO_PORT_B) is enough). */
void extiInit(void);

/* Volatile flag set by the ISRs. Read-and-clear from the main loop. */
extern volatile uint8_t exti_button_flag;

/* Convenience: atomically read and clear a flag. Returns previous value. */
uint8_t extiConsume(volatile uint8_t * flag);

#endif // STM32L4_EXTI_H
