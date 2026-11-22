#ifndef FPGA_PORT_H_INCLUDED
#define FPGA_PORT_H_INCLUDED

#include <stdint.h>

/* Hardware hooks fpga_link.c needs. fpga_port_stm32.c implements them with the
 * lib/ drivers; test/fpga_port_fake.c implements them with a bit-level model of
 * Pierce's BNN_top/BNN_spi, so the protocol is tested on the laptop. */

void     fpga_port_init(void);              /* pins as outputs (all low), us timer */
void     fpga_port_load(int high);          /* PA4 -> FPGA_A load, ACTIVE HIGH */
void     fpga_port_load_weights(int high);  /* PA1 -> FPGA_A load_weights */
void     fpga_port_clear(int high);         /* PA3 -> FPGA_A clear (handled in our VGA/link RTL) */
uint8_t  fpga_port_xfer(uint8_t out);       /* 8 SCK clocks: send a byte, return the byte read */
uint32_t fpga_port_millis(void);            /* millisecond clock, for timeouts */
void     fpga_port_delay_us(uint32_t us);   /* busy-wait at least `us` microseconds */

#endif