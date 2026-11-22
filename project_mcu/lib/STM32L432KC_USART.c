// STM32L432KC_USART.c
// USART driver implementation.
//
// Ported from E155 Lab 7 by Haspard Murphy. See header for the change list.
// USART register layout and field meanings: RM0394 Chapter 38 (USART).

#include "STM32L432KC.h"
#include "STM32L432KC_USART.h"
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_RCC.h"
#include "pins.h"

USART_TypeDef * id2Port(int USART_ID) {
    switch (USART_ID) {
        case USART1_ID: return USART1;
        case USART2_ID: return USART2;
    }
    return 0;
}

USART_TypeDef * initUSART(int USART_ID, int baud_rate) {
    USART_TypeDef * USART = id2Port(USART_ID);
    if (USART == 0) return 0;

    // Enable GPIOA clock (both USART1 PA9/PA10 and USART2 PA2/PA15 live on A).
    gpioEnable(GPIO_PORT_A);

    uint32_t clk = PCLK1_FREQ;                          // reset mux = PCLK

    switch (USART_ID) {
        case USART1_ID:
            // VERIFY: this file project assumes USART2; USART1 path kept for parity with lab 7.
            RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
            RCC->CCIPR   &= ~RCC_CCIPR_USART1SEL;        // 00 = PCLK2
            clk = PCLK2_FREQ;
            pinMode(PA9,  GPIO_ALT);
            pinMode(PA10, GPIO_ALT);
            pinAF (PA9,  7);                             // USART1_TX AF7
            pinAF (PA10, 7);                             // USART1_RX AF7
            break;
        case USART2_ID:
            RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;
            RCC->CCIPR    &= ~RCC_CCIPR_USART2SEL;       // 00 = PCLK1
            clk = PCLK1_FREQ;
            pinMode(PIN_UART_TX, GPIO_ALT);
            pinMode(PIN_UART_RX, GPIO_ALT);
            pinAF (PIN_UART_TX, UART_AF_TX);             // PA2 AF7
            pinAF (PIN_UART_RX, UART_AF_RX);             // PA15 AF3 (VERIFY DS11451 Table 15)
            pinResistor(PIN_UART_RX, GPIO_PULL_UP);      // idle-high line
            break;
    }

    // 8N1, 16x oversampling, UE off while reconfiguring.
    USART->CR1 = 0;
    USART->CR2 = 0;
    USART->CR3 = 0;

    // Baud: USARTDIV = f_CK / baud when OVER8 = 0 (RM0394 38.5.4).
    // 80e6 / 115200 = 694.44 -> 694 (0.064% error).
    USART->BRR = (uint16_t)(clk / (uint32_t)baud_rate);

    USART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    return USART;
}

void sendChar(USART_TypeDef * USART, char data) {
    while (!(USART->ISR & USART_ISR_TXE));
    USART->TDR = (uint8_t)data;
}

char readChar(USART_TypeDef * USART) {
    while (!(USART->ISR & USART_ISR_RXNE));
    return (char)(USART->RDR & 0xFF);
}

void sendString(USART_TypeDef * USART, const char * str) {
    while (*str) sendChar(USART, *str++);
}

int usartRxReady(USART_TypeDef * USART) {
    return (USART->ISR & USART_ISR_RXNE) ? 1 : 0;
}

////////////////////////////////////////////////////////////////////////////////
// USART2 shortcuts for rest of firmware
////////////////////////////////////////////////////////////////////////////////

void uart_putc(char c) {
    sendChar(USART2, c);
}

void uart_puts(const char * s) {
    while (*s) uart_putc(*s++);
}

/* Unsigned decimal, no padding. */
void uart_print_u32(uint32_t v) {
    char buf[11];
    int i = 0;
    if (v == 0) { uart_putc('0'); return; }
    while (v) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    while (i) uart_putc(buf[--i]);
}

void uart_print_hex8(uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    uart_putc(hex[(v >> 4) & 0xF]);
    uart_putc(hex[ v       & 0xF]);
}
