#ifndef FPGA_PORT_FAKE_H_INCLUDED
#define FPGA_PORT_FAKE_H_INCLUDED

#include <stdint.h>
#include "bitmap.h"

/* Laptop stand-in for FPGA_A: a bit-level C model of Pierce's BNN_spi.sv and
 * the load/load_weights handling in BNN_top.sv + Memory.sv. The BNN core
 * itself is the C golden model. */

typedef enum {
    FAKE_NORMAL,        /* core finishes ~350 us after load falls */
    FAKE_NEVER_DONE,    /* core never finishes */
    FAKE_BAD_CLASS,     /* core reports class 0x7F */
} fake_mode_t;

extern fake_mode_t fake_mode;

extern uint32_t fake_weight_words;      /* words written to the weight SPRAM */
extern uint32_t fake_weight_mismatch;   /* of those, how many differ from bnn_weights[] */
extern uint32_t fake_image_words;       /* words written by the last image transfer */
extern uint64_t fake_image[16];         /* the activation EBR's input words */
extern int      fake_partial_words;     /* transfers that ended mid-word */
extern int      fake_gap_violations;    /* load/load_weights edges within 1 us of SCK */
extern int      fake_runs;              /* times the core started */
extern int      fake_clear_pulses;      /* clear pulses at least 10 us wide */

void fake_reset(void);

#endif