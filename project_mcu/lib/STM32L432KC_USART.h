// STM32L432KC_USART.h
// USART driver. Primary target is USART2 on PA2/PA15 at 115200 8N1 to the
// ST-Link virtual COM port.
//
// Ported from E155 Lab 7 by Haspard Murphy. Changes vs lab 7:
//   - Clock source left at reset (PCLK1 = 80 MHz) instead of HSI16 override
//   - BRR now computed from PCLK (80 MHz) in the .c
//   - Added char_out_function-compatible shortcuts uart_putc() / uart_puts()
//     that target USART2 directly so they can be passed to bitmap_print().

#ifndef STM32L4_USART_H
#define STM32L4_USART_H

#include <stdint.h>
#include <stm32l432xx.h>

#define USART1_ID   1
#define USART2_ID   2

USART_TypeDef * id2Port(int USART_ID);

/* Initialize USART_ID at baud_rate. Returns pointer to the peripheral. */
USART_TypeDef * initUSART(int USART_ID, int baud_rate);

/* Generic helpers (any USART). */
void sendChar(USART_TypeDef * USART, char data);
char readChar(USART_TypeDef * USART);
void sendString(USART_TypeDef * USART, const char * str);
int  usartRxReady(USART_TypeDef * USART);   // 1 if a byte is waiting

/* USART2 shortcuts. uart_putc matches char_out_function in bitmap_print.h. */
void uart_putc(char c);
void uart_puts(const char * s);
void uart_print_u32(uint32_t v);
void uart_print_hex8(uint8_t v);

#endif // STM32L4_USART_H
