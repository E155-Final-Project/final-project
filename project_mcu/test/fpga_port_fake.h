#ifndef FPGA_PORT_FAKE_H
#define FPGA_PORT_FAKE_H

#include <stdint.h>
#include "bitmap.h"

/* A simulated FPGA_A on the far end of fpga_port_*. It decodes each CS frame
 * the way Pierce's SPI slave must, and runs the C golden model on bitmaps. */

typedef enum {
    FAKE_NORMAL,        /* raise done, return the golden-model class */
    FAKE_NEVER_DONE,    /* never raise done */
    FAKE_NO_RESULT,     /* raise done, return 0xFF */
} fake_mode_t;

extern fake_mode_t fake_mode;

extern int      fake_frames;            /* CS-low periods seen */
extern int      fake_bad_frames;        /* frames with a wrong length or unknown command */
extern int      fake_clears;
extern int      fake_weights_ok;        /* LOAD_WEIGHTS payload matched bnn_weights[] */
extern int      fake_thresh_ok;
extern uint64_t fake_last_bitmap[BITMAP_WORDS];  /* as reassembled from the wire */

void fake_reset(void);

#endif