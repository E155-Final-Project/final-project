// STM32L432KC_SPI.h
// SPI1 master, pins PA5 (SCK) / PA6 (MISO) / PA7 (MOSI), AF5.
// CS is a plain GPIO on PA4 driven manually by the caller (initSPI leaves it
// configured as an output and deasserted HIGH).
//
// Ported from E155 Lab 7 by Haspard Murphy. Changes vs lab 7:
//   - Pins moved from PB3/PB4/PB5 to PA5/PA6/PA7 so PB3 can be LD3.
//   - Pin assignments live in pins.h, not here.
//   - Added spiCSLow/spiCSHigh helpers for the "load" line.

#ifndef STM32L4_SPI_H
#define STM32L4_SPI_H

#include <stdint.h>
#include <stm32l432xx.h>

/* Initialize SPI1 as master, Mode (cpol, cpha), 8-bit frames.
 *   br: baud rate prescaler field (0b000..0b111). SPI clk = PCLK2 / 2^(br+1).
 *       With PCLK2 = 80 MHz, br=3 gives 5 MHz, br=2 gives 10 MHz, br=4 gives 2.5 MHz.
 *   cpol, cpha: 0/1 each. Mode 0 is cpol=0, cpha=0.                        */
void initSPI(int br, int cpol, int cpha);

/* Full-duplex byte exchange. Blocks on TXE then RXNE. */
uint8_t spiSendReceive(uint8_t send);

/* Chip-select helpers for the "load" line (PIN_SPI_CS). */
void spiCSLow(void);
void spiCSHigh(void);

#endif // STM32L4_SPI_H
