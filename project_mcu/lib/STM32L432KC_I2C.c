// STM32L432KC_I2C.c
// I2C1 master driver. 400 kHz, open-drain, blocking transfers with timeouts.
//
// Written for the E155 final project by Haspard Murphy.
//
// TIMINGR derivation (RM0394 39.4.10):
//   I2CCLK source = PCLK1 = 80 MHz   (CCIPR.I2C1SEL = 00, reset default)
//   PRESC = 7  -> t_PRESC  = 8 / 80 MHz = 100 ns
//   SCLL  = 0x11 (17) -> t_LOW  = (17+1) * 100 ns = 1800 ns  (FS min 1300 ns) OK
//   SCLH  = 0x07 (7)  -> t_HIGH = (7 +1) * 100 ns =  800 ns  (FS min  600 ns) OK
//   SCLDEL = 3 -> data setup = 4 * 100 ns = 400 ns  (FS min 100 ns) OK
//   SDADEL = 2 -> data hold  = 2 * 100 ns = 200 ns
//   SCL period ~= (18 + 8) * 100 ns = 2600 ns  ->  ~385 kHz (safely under 400k)
// TIMINGR = (7<<28)|(3<<20)|(2<<16)|(7<<8)|0x11 = 0x70320711
// VERIFY: against the STM32CubeMX I2C timing tool once hardware is in hand.

#include "STM32L432KC.h"
#include "STM32L432KC_I2C.h"
#include "STM32L432KC_GPIO.h"
#include "STM32L432KC_SYSTICK.h"
#include "pins.h"

#define I2C1_TIMINGR_400K   0x70320711u

// Per-wait timeout. Transfers of a few bytes at 400 kHz take microseconds, but
// a wedged bus can hang forever -- this bounds it to 10 ms.
#define I2C_TIMEOUT_MS      10u

/* Return non-zero when the condition went true before the timeout.
 * Checks NACK/BERR/ARLO along the way and short-circuits. */
static int waitFor(uint32_t isr_mask, uint32_t deadline, int * err) {
    for (;;) {
        uint32_t isr = I2C1->ISR;
        if (isr & I2C_ISR_NACKF) { *err = I2C_ERR_NACK; return 0; }
        if (isr & (I2C_ISR_BERR | I2C_ISR_ARLO)) { *err = I2C_ERR_BUS; return 0; }
        if (isr & isr_mask) return 1;
        if ((int32_t)(millis() - deadline) >= 0) { *err = I2C_ERR_TIMEOUT; return 0; }
    }
}

/* Flush any stuck condition and return bus flags to a clean state. */
static void busCleanup(void) {
    I2C1->ICR = I2C_ICR_NACKCF | I2C_ICR_BERRCF | I2C_ICR_ARLOCF |
                I2C_ICR_STOPCF | I2C_ICR_ADDRCF;
    // STOP will re-release SCL/SDA. If a transfer is still half-live, issue one.
    if (I2C1->ISR & I2C_ISR_BUSY) {
        I2C1->CR2 |= I2C_CR2_STOP;
        uint32_t deadline = millis() + I2C_TIMEOUT_MS;
        while ((I2C1->ISR & I2C_ISR_BUSY) &&
               ((int32_t)(millis() - deadline) < 0));
        I2C1->ICR = I2C_ICR_STOPCF;
    }
}

void i2cInit(void) {
    // GPIOA + I2C1 clocks.
    RCC->AHB2ENR  |= RCC_AHB2ENR_GPIOAEN;
    RCC->APB1ENR1 |= RCC_APB1ENR1_I2C1EN;

    // I2C1SEL = 00 -> PCLK1 (reset default; make it explicit).
    RCC->CCIPR &= ~RCC_CCIPR_I2C1SEL;

    // SCL / SDA: AF4, open-drain, internal pull-up (keeps the line defined if
    // no external pull-ups are fitted. Trill breakouts usually include 2.2k).
    pinMode    (PIN_I2C_SCL, GPIO_ALT);
    pinMode    (PIN_I2C_SDA, GPIO_ALT);
    pinAF      (PIN_I2C_SCL, I2C_AF);
    pinAF      (PIN_I2C_SDA, I2C_AF);
    pinOutputType(PIN_I2C_SCL, GPIO_OPEN_DRAIN);
    pinOutputType(PIN_I2C_SDA, GPIO_OPEN_DRAIN);
    pinResistor(PIN_I2C_SCL, GPIO_PULL_UP);
    pinResistor(PIN_I2C_SDA, GPIO_PULL_UP);

    // Program peripheral.
    I2C1->CR1     = 0;                          // disable to edit
    I2C1->TIMINGR = I2C1_TIMINGR_400K;
    I2C1->CR2     = 0;
    I2C1->CR1     = I2C_CR1_PE;                 // 7-bit addressing, no filters
}

static int startTransfer(uint8_t addr7, uint16_t n, uint32_t extra_cr2) {
    // Write CR2 in one go: SADD, dir, NBYTES, optional AUTOEND/RELOAD, START.
    uint32_t cr2 = ((uint32_t)addr7 << 1) & I2C_CR2_SADD_Msk;
    cr2 |= ((uint32_t)n << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES_Msk;
    cr2 |= extra_cr2 | I2C_CR2_START;
    I2C1->CR2 = cr2;
    return I2C_OK;
}

int i2cWrite(uint8_t addr7, const uint8_t * data, uint16_t n) {
    int err = I2C_OK;
    uint32_t deadline = millis() + I2C_TIMEOUT_MS;
    busCleanup();

    startTransfer(addr7, n, (n > 0) ? I2C_CR2_AUTOEND : I2C_CR2_AUTOEND);

    for (uint16_t i = 0; i < n; i++) {
        if (!waitFor(I2C_ISR_TXIS, deadline, &err)) { busCleanup(); return err; }
        I2C1->TXDR = data[i];
    }
    if (!waitFor(I2C_ISR_STOPF, deadline, &err)) { busCleanup(); return err; }
    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}

int i2cRead(uint8_t addr7, uint8_t * data, uint16_t n) {
    int err = I2C_OK;
    uint32_t deadline = millis() + I2C_TIMEOUT_MS;
    busCleanup();

    startTransfer(addr7, n, I2C_CR2_RD_WRN | I2C_CR2_AUTOEND);

    for (uint16_t i = 0; i < n; i++) {
        if (!waitFor(I2C_ISR_RXNE, deadline, &err)) { busCleanup(); return err; }
        data[i] = (uint8_t)I2C1->RXDR;
    }
    if (!waitFor(I2C_ISR_STOPF, deadline, &err)) { busCleanup(); return err; }
    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}

int i2cWriteRead(uint8_t addr7,
                 const uint8_t * wdata, uint16_t wn,
                 uint8_t * rdata, uint16_t rn) {
    int err = I2C_OK;
    uint32_t deadline = millis() + I2C_TIMEOUT_MS;
    busCleanup();

    // Phase 1: write wn bytes WITHOUT auto-end -- we will issue a repeated START.
    startTransfer(addr7, wn, 0);
    for (uint16_t i = 0; i < wn; i++) {
        if (!waitFor(I2C_ISR_TXIS, deadline, &err)) { busCleanup(); return err; }
        I2C1->TXDR = wdata[i];
    }
    // Wait for the transfer-complete flag before switching direction.
    if (!waitFor(I2C_ISR_TC, deadline, &err)) { busCleanup(); return err; }

    // Phase 2: repeated START for the read.
    startTransfer(addr7, rn, I2C_CR2_RD_WRN | I2C_CR2_AUTOEND);
    for (uint16_t i = 0; i < rn; i++) {
        if (!waitFor(I2C_ISR_RXNE, deadline, &err)) { busCleanup(); return err; }
        rdata[i] = (uint8_t)I2C1->RXDR;
    }
    if (!waitFor(I2C_ISR_STOPF, deadline, &err)) { busCleanup(); return err; }
    I2C1->ICR = I2C_ICR_STOPCF;
    return I2C_OK;
}
