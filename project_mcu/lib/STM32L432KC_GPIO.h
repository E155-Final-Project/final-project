// STM32L432KC_GPIO.h
// GPIO driver: mode, alternate function, pull resistors, output type,
// digital read/write, toggle.
//
// Ported from E155 Lab 7 by Haspard Murphy. Added pinResistor() definition
// (was declared but missing), pinOutputType(), and pinAF() helper.

#ifndef STM32L4_GPIO_H
#define STM32L4_GPIO_H

#include <stdint.h>
#include <stm32l432xx.h>

///////////////////////////////////////////////////////////////////////////////
// Definitions
///////////////////////////////////////////////////////////////////////////////

// digitalWrite() values
#define PIO_LOW   0
#define PIO_HIGH  1

// Port IDs for gpioEnable()
#define GPIO_PORT_A 0
#define GPIO_PORT_B 1
#define GPIO_PORT_C 2

// pinMode() modes
#define GPIO_INPUT  0
#define GPIO_OUTPUT 1
#define GPIO_ALT    2
#define GPIO_ANALOG 3

// pinResistor() settings
#define GPIO_PULL_UP   0
#define GPIO_PULL_DOWN 1
#define GPIO_FLOATING  2

// pinOutputType() settings
#define GPIO_PUSH_PULL  0
#define GPIO_OPEN_DRAIN 1

// Flat pin IDs: upper 4 bits = port, lower 4 bits = pin number within port.
#define PA0    0
#define PA1    1
#define PA2    2
#define PA3    3
#define PA4    4
#define PA5    5
#define PA6    6
#define PA7    7
#define PA8    8
#define PA9    9
#define PA10   10
#define PA11   11
#define PA12   12
#define PA13   13
#define PA14   14
#define PA15   15
#define PB0    16
#define PB1    17
#define PB2    18
#define PB3    19
#define PB4    20
#define PB5    21
#define PB6    22
#define PB7    23
#define PB8    24
#define PB9    25
#define PB10   26
#define PB11   27
#define PB12   28
#define PB13   29
#define PB14   30
#define PB15   31
#define PC0    32
#define PC1    33
#define PC2    34
#define PC3    35
#define PC4    36
#define PC5    37
#define PC6    38
#define PC7    39
#define PC8    40
#define PC9    41
#define PC10   42
#define PC11   43
#define PC12   44
#define PC13   45
#define PC14   46
#define PC15   47

///////////////////////////////////////////////////////////////////////////////
// Function prototypes
///////////////////////////////////////////////////////////////////////////////

void gpioEnable(int port_id);
int  gpioPinOffset(int gpio_pin);
int  gpioPinToPort(int gpio_pin);
GPIO_TypeDef * gpioPortToBase(int port);
GPIO_TypeDef * gpioPinToBase(int gpio_pin);

void pinMode(int gpio_pin, int function);
void pinResistor(int gpio_pin, int setting);
void pinOutputType(int gpio_pin, int setting);
void pinAF(int gpio_pin, int af);          // af in 0..15, writes AFR[L|H]

int  digitalRead(int gpio_pin);
void digitalWrite(int gpio_pin, int val);
void togglePin(int gpio_pin);

#endif // STM32L4_GPIO_H
