// STM32L432KC_GPIO.c
// GPIO driver implementation.
//
// Ported from E155 Lab 7 by Haspard Murphy. Added pinResistor(),
// pinOutputType(), and pinAF() helper. Register bit locations per
// RM0394 Section 8 (GPIO registers).

#include "STM32L432KC_RCC.h"
#include "STM32L432KC_GPIO.h"

////////////////////////////////////////////////////////////////////////////////
// Port / pin helpers
////////////////////////////////////////////////////////////////////////////////

void gpioEnable(int port_id) {
    switch (port_id) {
        case GPIO_PORT_A: RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN; break;
        case GPIO_PORT_B: RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN; break;
        case GPIO_PORT_C: RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN; break;
    }
}

int gpioPinOffset(int gpio_pin) {
    return gpio_pin & 0x0F;             // low 4 bits = pin within port
}

int gpioPinToPort(int gpio_pin) {
    return gpio_pin >> 4;               // high 4 bits = port id
}

GPIO_TypeDef * gpioPortToBase(int port) {
    switch (port) {
        case GPIO_PORT_A: return (GPIO_TypeDef *) GPIOA_BASE;
        case GPIO_PORT_B: return (GPIO_TypeDef *) GPIOB_BASE;
        case GPIO_PORT_C: return (GPIO_TypeDef *) GPIOC_BASE;
    }
    return 0;
}

GPIO_TypeDef * gpioPinToBase(int gpio_pin) {
    return gpioPortToBase(gpioPinToPort(gpio_pin));
}

////////////////////////////////////////////////////////////////////////////////
// Configuration
////////////////////////////////////////////////////////////////////////////////

/* Set pin mode: INPUT / OUTPUT / ALT / ANALOG.
 * MODER is 2 bits per pin (RM0394 8.4.1). */
void pinMode(int gpio_pin, int function) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);

    port->MODER &= ~(0b11u << (2 * off));
    switch (function) {
        case GPIO_INPUT:                                                break;
        case GPIO_OUTPUT: port->MODER |= (0b01u << (2 * off));          break;
        case GPIO_ALT:    port->MODER |= (0b10u << (2 * off));          break;
        case GPIO_ANALOG: port->MODER |= (0b11u << (2 * off));          break;
    }
}

/* Pull-up / pull-down / floating.
 * PUPDR is 2 bits per pin (RM0394 8.4.4). */
void pinResistor(int gpio_pin, int setting) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);

    port->PUPDR &= ~(0b11u << (2 * off));
    switch (setting) {
        case GPIO_PULL_UP:   port->PUPDR |= (0b01u << (2 * off));       break;
        case GPIO_PULL_DOWN: port->PUPDR |= (0b10u << (2 * off));       break;
        case GPIO_FLOATING:                                             break;
    }
}

/* Push-pull or open-drain output type.
 * OTYPER is 1 bit per pin (RM0394 8.4.2). */
void pinOutputType(int gpio_pin, int setting) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);

    if (setting == GPIO_OPEN_DRAIN) port->OTYPER |=  (1u << off);
    else                            port->OTYPER &= ~(1u << off);
}

/* Select alternate function 0..15.
 * AFR[0] covers pins 0..7, AFR[1] covers pins 8..15, 4 bits per pin
 * (RM0394 8.4.9, 8.4.10). */
void pinAF(int gpio_pin, int af) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);
    int idx = off >> 3;                 // 0 for pins 0..7, 1 for pins 8..15
    int shift = (off & 0x7) * 4;

    port->AFR[idx] &= ~(0xFu << shift);
    port->AFR[idx] |= ((uint32_t)(af & 0xF)) << shift;
}

////////////////////////////////////////////////////////////////////////////////
// Read / write
////////////////////////////////////////////////////////////////////////////////

int digitalRead(int gpio_pin) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);
    return (int)((port->IDR >> off) & 1u);
}

void digitalWrite(int gpio_pin, int val) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);
    // BSRR: set bits in low half, reset bits in high half (RM0394 8.4.7).
    // Atomic, avoids a read-modify-write race with an ISR touching the same port.
    if (val) port->BSRR = (1u << off);
    else     port->BSRR = (1u << (off + 16));
}

void togglePin(int gpio_pin) {
    GPIO_TypeDef * port = gpioPinToBase(gpio_pin);
    int off = gpioPinOffset(gpio_pin);
    port->ODR ^= (1u << off);
}
