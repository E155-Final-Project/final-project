// STM32L432KC_EXTI.c
// External interrupt setup and handlers.
//
// Written for the E155 final project by Haspard Murphy.
// Register references: RM0394 Chapter 14 (EXTI) and Chapter 9.2 (SYSCFG EXTICRn).

#include "STM32L432KC.h"
#include "STM32L432KC_EXTI.h"
#include "STM32L432KC_GPIO.h"
#include "pins.h"
#include "fpga_link.h"

volatile uint8_t exti_button_flag = 0;

uint8_t extiConsume(volatile uint8_t * flag) {
    // SysTick (and any other IRQ at same priority) could touch this; disable
    // interrupts across the read-modify-write to be safe. The critical section
    // is a handful of cycles so it does not meaningfully affect latency.
    __asm__ volatile ("cpsid i" ::: "memory");
    uint8_t v = *flag;
    *flag = 0;
    __asm__ volatile ("cpsie i" ::: "memory");
    return v;
}

void extiInit(void) {
    // GPIOB clock (PIN_FPGA_DONE = PB0, PIN_BTN_CLEAR = PB1).
    gpioEnable(GPIO_PORT_B);

    // Pins as inputs. done gets a pull-down so it reads low (no false edges)
    // while the FPGA isn't driving it. Button needs a pull-up (button to GND).
    pinMode(PIN_FPGA_DONE, GPIO_INPUT);
    pinResistor(PIN_FPGA_DONE, GPIO_PULL_DOWN);
    pinMode(PIN_BTN_CLEAR, GPIO_INPUT);
    pinResistor(PIN_BTN_CLEAR, GPIO_PULL_UP);

    

    // Enable SYSCFG so EXTICR writes land.
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

    // Route EXTI0 to PB0 and EXTI1 to PB1.
    // EXTICR[0] covers EXTI lines 0..3, 4 bits per line. 0b0001 = Port B.
    SYSCFG->EXTICR[0] &= ~((0xFu << 0) | (0xFu << 4));
    SYSCFG->EXTICR[0] |=  ((0x1u << 0) | (0x1u << 4));

    // Triggers: EXTI0 rising (done), EXTI1 falling (button press -> GND).
    EXTI->RTSR1 |=  (1u << 0);
    EXTI->RTSR1 &= ~(1u << 1);
    EXTI->FTSR1 |=  (1u << 1);
    EXTI->FTSR1 &= ~(1u << 0);

    // Clear any stale pending bits (W1C).
    EXTI->PR1 = (1u << 0) | (1u << 1);

    // Unmask both lines.
    EXTI->IMR1 |= (1u << 0) | (1u << 1);

    // NVIC: EXTI0_IRQn = 6, EXTI1_IRQn = 7 (RM0394 Table 46).
    NVIC_EnableIRQ(EXTI0_IRQn);
    NVIC_EnableIRQ(EXTI1_IRQn);
}

// Vector names from stm32l432xx_Vectors.s.
void EXTI0_IRQHandler(void) {
    if (EXTI->PR1 & (1u << 0)) {
        EXTI->PR1 = (1u << 0);                          // W1C
        fpga_link_done_isr();                             // set done_flag
    }
}

void EXTI1_IRQHandler(void) {
    if (EXTI->PR1 & (1u << 1)) {
        EXTI->PR1 = (1u << 1);                          // W1C
        exti_button_flag = 1;
    }
}
