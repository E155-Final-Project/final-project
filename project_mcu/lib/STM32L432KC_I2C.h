// STM32L432KC_I2C.h
// I2C1 master driver. 400 kHz Fast Mode, open-drain on PA9/PA10.
// Blocking write / read / write-then-read with repeated start.
// Returns an error code instead of hanging on NACK or bus hangup.
//
// Written for the E155 final project by Haspard Murphy.
// Register references: RM0394 Chapter 39 (I2C).

#ifndef STM32L4_I2C_H
#define STM32L4_I2C_H

#include <stdint.h>

/* Return codes for the blocking transfer functions. */
#define I2C_OK          0
#define I2C_ERR_NACK    1       // peripheral reported a NACK during AF
#define I2C_ERR_TIMEOUT 2       // a wait loop hit its budget
#define I2C_ERR_BUS     3       // BERR or ARLO on the bus

/* Enable I2C1 clock, configure PA9/PA10 AF4 open-drain, load TIMINGR for
 * ~400 kHz at 80 MHz PCLK1. */
void i2cInit(void);

/* Write n bytes to a 7-bit address. */
int i2cWrite(uint8_t addr7, const uint8_t * data, uint16_t n);

/* Read n bytes from a 7-bit address. */
int i2cRead(uint8_t addr7, uint8_t * data, uint16_t n);

/* Write wn bytes then (with repeated start) read rn bytes.
 * This is the "set register, read value" pattern used by Trill. */
int i2cWriteRead(uint8_t addr7,
                 const uint8_t * wdata, uint16_t wn,
                 uint8_t * rdata, uint16_t rn);

#endif // STM32L4_I2C_H
