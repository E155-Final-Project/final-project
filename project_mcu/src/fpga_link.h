#ifndef FPGA_LINK_H_INCLUDED
#define FPGA_LINK_H_INCLUDED

#include <stdint.h>
#include "bitmap.h"

/* MCU -> FPGA_A link, matching Pierce's BNN_top.sv / BNN_spi.sv and his
 * tb_bnn_spi.sv. No command bytes: two level pins pick what a transfer is.
 *
 *   SPI Mode 0, 5 MHz, LSB first. load is ACTIVE HIGH (idle low).
 *
 *   Weights:  load_weights=1, load=1, 121,792 B (15,224 words), load=0, load_weights=0
 *   Classify: load=1, 104 B (13 words, last 48 bits 0), load=0 -> BNN starts
 *             wait for done (PB0 rising), then 8 clocks with load LOW -> class
 *   Clear:    pulse the clear pin (FPGA_A side: our VGA + FPGA_B link RTL)
 *
 * Every load/load_weights edge is at least FPGA_EDGE_US from the nearest SCK
 * edge, like the testbench: the FPGA syncs these pins into its 12 MHz clock,
 * and the last word must be written before load_weights drops. */

#define FPGA_IMAGE_BYTES      BITMAP_BYTES   /* 104 */
#define FPGA_EDGE_US          1u
#define FPGA_CLEAR_PULSE_US   10u
#define FPGA_DONE_TIMEOUT_MS  5u             /* one inference is ~0.35 ms per the tb */

typedef enum {
    FPGA_OK = 0,
    FPGA_ERR_TIMEOUT,     /* done never rose after the image */
    FPGA_ERR_BAD_CLASS,   /* class read back is above 46 */
} fpga_status_t;

void fpga_link_init(void);                 /* all three pins low, done flag cleared */
void fpga_link_done_isr(void);             /* from the EXTI handler for done (PB0) */

fpga_status_t fpga_load_weights(void);     /* once at boot, from flash */
fpga_status_t fpga_classify(const uint64_t bm[BITMAP_WORDS], uint8_t *cls);
fpga_status_t fpga_clear(void);

/* Loopback self-test: jumper MOSI (PA7) to MISO (PA6), FPGA disconnected.
 * Returns the number of mismatched bytes (0 = pass). */
int fpga_link_loopback_test(const uint64_t bm[BITMAP_WORDS]);

const char *fpga_status_str(fpga_status_t s);

#endif