// pins.h
// Central pin map for the E155 final-project MCU (NUCLEO-L432KC, STM32L432KCU).
// Change pins HERE, not in the drivers.

#ifndef PINS_H
#define PINS_H

#include "STM32L432KC_GPIO.h"   // PA0..PC15 integer IDs

///////////////////////////////////////////////////////////////////////////////
// Board constraints -- do not reassign without checking UM1956 and DS11451
///////////////////////////////////////////////////////////////////////////////

// Fixed by the ST-Link virtual COM port:
//   PA2  = USART2_TX  (AF7)
//   PA15 = USART2_RX  (AF3)
// Fixed by the board (soldered):
//   PB3  = LD3 user LED
//   PA13 = SWDIO, PA14 = SWCLK  -- NEVER reconfigure

// NUCLEO-L432KC default solder bridges (UM1956, user-confirmed):
//   SB18 closed: Arduino D4 (PB7) is shorted to A4 (PA5)
//   SB16 closed: Arduino D5 (PB6) is shorted to A5 (PA6)
// Consequences for this project:
//   - SPI1 on PA5/PA6/PA7 is OK, but PB6/PB7 must be left at reset (floating
//     input). Do NOT configure PB6 or PB7 as outputs anywhere in firmware.
//   - I2C1 on PB6/PB7 would conflict with SPI; we route I2C1 to PA9/PA10
//     instead.
//   - Do not connect anything to CN4 D4/D5 header pins on the breadboard --
//     SPI SCK/MISO will appear there too.

///////////////////////////////////////////////////////////////////////////////
// UART (ST-Link VCP, 115200 8N1)
///////////////////////////////////////////////////////////////////////////////
#define PIN_UART_TX     PA2     // USART2_TX, AF7      -- VERIFY: DS11451 Table 15
#define PIN_UART_RX     PA15    // USART2_RX, AF3      -- VERIFY: DS11451 Table 15
#define UART_BAUD       115200
#define UART_AF_TX      7
#define UART_AF_RX      3

///////////////////////////////////////////////////////////////////////////////
// LED (LD3 on Nucleo)
///////////////////////////////////////////////////////////////////////////////
#define PIN_LED         PB3     // LD3, driven active-high

///////////////////////////////////////////////////////////////////////////////
// SPI1 (master, Mode 0, ~5 MHz, full-duplex)
///////////////////////////////////////////////////////////////////////////////
#define PIN_SPI_SCK     PA5     // SPI1_SCK,  AF5      -- VERIFY: DS11451 Table 15
#define PIN_SPI_MISO    PA6     // SPI1_MISO, AF5      -- VERIFY
#define PIN_SPI_MOSI    PA7     // SPI1_MOSI, AF5      -- VERIFY
#define PIN_SPI_CS      PA4     // Plain GPIO output ("load" / CS)
#define SPI_AF          5

///////////////////////////////////////////////////////////////////////////////
// I2C1 (400 kHz master, open-drain)
///////////////////////////////////////////////////////////////////////////////
#define PIN_I2C_SCL     PA9     // I2C1_SCL, AF4       -- VERIFY: DS11451 Table 15
#define PIN_I2C_SDA     PA10    // I2C1_SDA, AF4       -- VERIFY
#define I2C_AF          4

// Trill Square 7-bit address                           -- VERIFY: Trill docs
#define TRILL_I2C_ADDR  0x28

///////////////////////////////////////////////////////////////////////////////
// EXTI -- pin numbers chosen so each gets its own vector (EXTI0_IRQn, EXTI1_IRQn)
///////////////////////////////////////////////////////////////////////////////
#define PIN_FPGA_DONE   PB0     // EXTI0, rising edge
#define PIN_BTN_CLEAR   PB1     // EXTI1, falling edge, internal pull-up

#endif // PINS_H
