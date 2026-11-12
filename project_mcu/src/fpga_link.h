#ifndef FPGA_LINK_H
#define FPGA_LINK_H

#include <stdint.h>
#include "bitmap.h"

/* MCU -> FPGA_A SPI protocol (Section E).
 * SPI Mode 0, 5 MHz, LSB first. One command per CS-low period:
 *   [command byte][payload]  */
#define FPGA_CMD_CLASSIFY     0x01   /* + 98 B bitmap, pixel 0 first; FPGA raises done */
#define FPGA_CMD_READ_RESULT  0x02   /* + 1 dummy byte; class returned during it */
#define FPGA_CMD_CLEAR        0x03   /* no payload; FPGA blanks VGA, forwards to FPGA_B */
#define FPGA_CMD_LOAD_WEIGHTS 0x10   /* + 121,792 B: 15,224 words, LSB byte first */
#define FPGA_CMD_LOAD_THRESH  0x11   /* + 3,072 B: 1,536 thresholds, LSB byte first */

#define FPGA_RESULT_NONE      0xFF   /* READ_RESULT: no valid result */
#define FPGA_DONE_TIMEOUT_MS  5u

typedef enum {
    FPGA_OK = 0,
    FPGA_ERR_TIMEOUT,     /* done never rose after CLASSIFY */
    FPGA_ERR_NO_RESULT,   /* READ_RESULT returned 0xFF */
    FPGA_ERR_BAD_CLASS,   /* READ_RESULT returned a value above 46 */
} fpga_status_t;

void fpga_link_init(void);                  /* CS released, done flag cleared */

/* Call from the EXTI handler for the FPGA done pin (PB0, rising edge). */
void fpga_link_done_isr(void);

fpga_status_t fpga_load_weights(void);      /* weights then thresholds, from flash */
fpga_status_t fpga_classify(const uint64_t bm[BITMAP_WORDS], uint8_t *cls);
fpga_status_t fpga_clear(void);

/* Loopback self-test: jumper MOSI (PA7) to MISO (PA6), no FPGA attached.
 * Sends 256 test bytes plus the bitmap and checks each echo.
 * Returns the number of mismatched bytes (0 = pass). */
int fpga_link_loopback_test(const uint64_t bm[BITMAP_WORDS]);

const char *fpga_status_str(fpga_status_t s);

#endif