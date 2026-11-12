// main.c
// E155 final project -- MCU firmware hardware smoke test.
//
// What this build does:
//   1. Boot: configureClock() -> 80 MHz, SysTick 1 kHz, USART2 @ 115200 to the
//      ST-Link virtual COM port, LD3 as output, EXTI0/1 armed on PB0 / PB1.
//   2. In the main loop: blink LD3 at ~1 Hz and print "hello e155 <ms>\r\n"
//      once a second. A button press on PB1 prints a notice. A "done" rising
//      edge on PB0 also prints a notice.
//   3. Periodically runs an I2C scan (0x08..0x77) and prints ACKing addresses.
//   4. One-shot at boot: rasterize the hardcoded 'a' stroke and print the
//      28x28 bitmap over UART so bitmap_print() is exercised on-target.
//
// The portable src/ files (rasterize, bitmap_print, config.h, bitmap.h,
// classes, live_view) include nothing from lib/. Only main.c and future
// hardware modules reach into lib/.

#include <stdint.h>

#include "STM32L432KC.h"
#include "pins.h"

#include "bitmap.h"
#include "bitmap_print.h"
#include "rasterize.h"
#include "strokes_mcu.h"
#include "ui.h"



static void boardInit(void) {
    configureClock();                   // 80 MHz, 4 WS flash, PLL
    systickInit();                      // 1 kHz tick

    // Core peripherals.
    initUSART(USART2_ID, UART_BAUD);
    uart_puts("\r\n\r\n=== E155 final project MCU smoke test ===\r\n");
    uart_puts("SYSCLK=80MHz PCLK1=80MHz USART2 115200 8N1\r\n");

    // Port B: clear button (PB1, pull-p) and LED on LD3.
    gpioEnable  (GPIO_PORT_B);
    pinMode     (PIN_BTN_CLEAR, GPIO_INPUT);
    pinResistor (PIN_BTN_CLEAR, GPIO_PULL_UP);

    pinMode     (PIN_LED, GPIO_OUTPUT);
    digitalWrite(PIN_LED, PIO_LOW);

    // SPI1 master, Mode 0, ~5 MHz (BR=3 -> PCLK2/16 = 5 MHz at 80 MHz PCLK2).
    initSPI(/*br=*/3, /*cpol=*/0, /*cpha=*/0);

    // I2C1 400 kHz.
    i2cInit();

    // External interrupts (FPGA done rising on PB0, button falling on PB1).
    extiInit();
}

static void printRasterTest(void) {
    uart_puts("\r\n-- rasterize+bitmap_print self-test (letter 'a') --\r\n");

    uint64_t bm[BITMAP_WORDS];
    rast_stats_t stats;
    bitmap_clear(bm);

    int rc = rasterize(STROKE_A, STROKE_A_N, bm, &stats);
    uart_puts("rasterize rc="); uart_print_u32((uint32_t)rc);
    uart_puts(" ink=");         uart_print_u32((uint32_t)stats.ink);
    uart_puts(" rows=");        uart_print_u32((uint32_t)stats.box_rows);
    uart_puts(" cols=");        uart_print_u32((uint32_t)stats.box_cols);
    uart_puts("\r\n");

    bitmap_print(bm, uart_putc);
    uart_puts("-- end bitmap --\r\n\r\n");
}

static void i2cScan(void) {
    uart_puts("-- I2C scan 0x08..0x77 --\r\n");
    int found = 0;
    uint8_t dummy;
    (void)dummy;
    for (uint8_t a = 0x08; a <= 0x77; a++) {
        // Zero-byte write: just address phase. ACK -> device present, NACK -> not.
        int rc = i2cWrite(a, 0, 0);
        if (rc == I2C_OK) {
            uart_puts("  ACK @ 0x"); uart_print_hex8(a);
            if (a == TRILL_I2C_ADDR) uart_puts(" (Trill)");
            uart_puts("\r\n");
            found++;
        }
    }
    uart_puts("-- scan done, found="); uart_print_u32((uint32_t)found);
    uart_puts(" --\r\n\r\n");
}

int main(void)
{
    /* ...your existing init: clock, SysTick, GPIO, USART, SPI, EXTI... */
    ui_init();

    uint32_t last_beat = 0;

    for (;;) {
        uint32_t now = millis();

        /* Section G: button + LED */
        int pressed = !digitalRead(PIN_BTN_CLEAR);       /* active low */
        switch (ui_button_update(pressed, now)) {
        case UI_EVENT_SHORT: uart_puts("clear\r\n"); ui_led_pulse(now); break;
        case UI_EVENT_LONG:  uart_puts("long press: FPGA reset\r\n"); break;
        default: break;
        }
        digitalWrite(PIN_LED, ui_led_level(now));

        /* Heartbeat from the smoke test, kept non-blocking */
        if ((uint32_t)(now - last_beat) >= 1000) {
            last_beat = now;
            uart_puts("alive\r\n");
        }

        delay_ms(1);
    }
}

// SEGGER .init_array calls this; CMSIS provides a real implementation via
// system_stm32l4xx.c, so no local definition is needed here.
