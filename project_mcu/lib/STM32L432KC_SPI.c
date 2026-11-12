// STM32L432KC_SPI.c
// SPI1 master driver.
//
// Ported from E155 Lab 7 by Haspard Murphy. See header for the change list.
// Register layout / bits: RM0394 Chapter 40 (SPI).

#include "STM32L432KC.h"
#include "STM32L432KC_SPI.h"
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_RCC.h"
#include "pins.h"

void initSPI(int br, int cpol, int cpha) {
    // Clocks: GPIOA for pins, APB2 for SPI1.
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB2ENR  |= RCC_APB2ENR_SPI1EN;

    // SCK / MISO / MOSI -> AF5, high speed.
    pinMode(PIN_SPI_SCK,  GPIO_ALT);
    pinMode(PIN_SPI_MISO, GPIO_ALT);
    pinMode(PIN_SPI_MOSI, GPIO_ALT);
    pinAF  (PIN_SPI_SCK,  SPI_AF);
    pinAF  (PIN_SPI_MISO, SPI_AF);
    pinAF  (PIN_SPI_MOSI, SPI_AF);
    // OSPEEDR "very high speed" for SCK / MOSI so edges are clean at 5 MHz+.
    GPIOA->OSPEEDR |= (0b11u << (2 * 5)) | (0b11u << (2 * 7));

    // CS as a plain output, deasserted high.
    pinMode   (PIN_SPI_CS, GPIO_OUTPUT);
    digitalWrite(PIN_SPI_CS, PIO_HIGH);

    // Disable before configuring.
    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    // CR1: master, SSM+SSI (software NSS held high -> MSTR stays), BR = br,
    // CPOL / CPHA per arg. MSB first (LSBFIRST=0).
    SPI1->CR1 = SPI_CR1_MSTR
              | SPI_CR1_SSM | SPI_CR1_SSI
              | SPI_CR1_LSBFIRST
              | _VAL2FLD(SPI_CR1_BR,   (uint32_t)br)
              | _VAL2FLD(SPI_CR1_CPOL, (uint32_t)cpol)
              | _VAL2FLD(SPI_CR1_CPHA, (uint32_t)cpha);

    // CR2: 8-bit data (DS = 0b0111), RX FIFO threshold = quarter (byte access).
    SPI1->CR2 = _VAL2FLD(SPI_CR2_DS, 0b0111) | SPI_CR2_FRXTH;

    SPI1->CR1 |= SPI_CR1_SPE;                       // enable
}

uint8_t spiSendReceive(uint8_t send) {
    // Byte-wide access to DR to keep the FIFO threshold at 1 byte.
    while (!(SPI1->SR & SPI_SR_TXE));
    *((volatile uint8_t *)&SPI1->DR) = send;
    while (!(SPI1->SR & SPI_SR_RXNE));
    return *((volatile uint8_t *)&SPI1->DR);
}

void spiCSLow(void)  { digitalWrite(PIN_SPI_CS, PIO_LOW);  }
void spiCSHigh(void) { digitalWrite(PIN_SPI_CS, PIO_HIGH); }
