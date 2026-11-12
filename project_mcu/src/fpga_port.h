#ifndef FPGA_PORT_H
#define FPGA_PORT_H

#include <stdint.h>

void    fpga_port_cs(int active);      /* 1: pulls CS (PA4) low, 0: release high */
uint8_t fpga_port_xfer(uint8_t out);    /* send one byte, return byte received */
uint32_t fpga_port_millis(void);        /* milliseconds clk for timeout*/

#endif // FPGA_PORT_H