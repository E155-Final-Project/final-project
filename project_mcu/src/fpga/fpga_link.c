#include "fpga_link.h"
#include "fpga_port.h"
#include "classes.h"
#include "weights.h"

/* Set by the done-pin interrupt, cleared before each classify. */
static volatile int done_flag;

static void gap(void)
{
    fpga_port_delay_us(FPGA_EDGE_US);
}

void fpga_link_init(void)
{
    fpga_port_init();
    fpga_port_load(0);
    fpga_port_load_weights(0);
    fpga_port_clear(0);
    done_flag = 0;
}

void fpga_link_done_isr(void)
{
    done_flag = 1;
}

fpga_status_t fpga_load_weights(void)
{
    fpga_port_load_weights(1);
    gap();
    fpga_port_load(1);                       /* FPGA resets its word address */
    gap();
    for (uint32_t k = 0; k < BNN_WEIGHT_WORDS; k++) {
        uint64_t w = bnn_weights[k];
        for (int b = 0; b < 8; b++) {
            (void)fpga_port_xfer((uint8_t)(w >> (8 * b)));   /* LSB byte first */
        }
    }
    gap();                                   /* last word reaches SPRAM */
    fpga_port_load(0);
    gap();
    done_flag = 0;
    fpga_port_load_weights(0);               /* core leaves reset and runs once on
                                                a stale image; wait it out so its
                                                done edge can't race the next classify */
    uint32_t start = fpga_port_millis();
    while (!done_flag) {
        if ((uint32_t)(fpga_port_millis() - start) > FPGA_DONE_TIMEOUT_MS) {
            return FPGA_ERR_TIMEOUT;
        }
    }
    return FPGA_OK;
}

fpga_status_t fpga_classify(const uint64_t bm[BITMAP_WORDS], uint8_t *cls)
{
    /* Clear first: done stays high from the last result until load rises and
     * resets the core, and that is not a rising edge, so it can't set the flag. */
    done_flag = 0;

    fpga_port_load(1);                       /* core held in reset while high */
    gap();
    for (int i = 0; i < FPGA_IMAGE_BYTES; i++) {
        (void)fpga_port_xfer(bitmap_byte(bm, i));
    }
    gap();                                   /* word 12 written before the core starts */
    fpga_port_load(0);                       /* falling edge: the BNN starts */

    uint32_t start = fpga_port_millis();
    while (!done_flag) {
        if ((uint32_t)(fpga_port_millis() - start) > FPGA_DONE_TIMEOUT_MS) {
            return FPGA_ERR_TIMEOUT;
        }
    }

    /* 8 clocks with load still LOW. Raising load here would reset the core and
     * its falling edge would start another inference. */
    uint8_t r = fpga_port_xfer(0x00);
    gap();                                   /* keep the next load edge clear of SCK */
    if (r >= NUM_CLASSES) {
        return FPGA_ERR_BAD_CLASS;
    }
    *cls = r;
    return FPGA_OK;
}

fpga_status_t fpga_clear(void)
{
    fpga_port_clear(1);
    fpga_port_delay_us(FPGA_CLEAR_PULSE_US);
    fpga_port_clear(0);
    return FPGA_OK;
}

int fpga_link_loopback_test(const uint64_t bm[BITMAP_WORDS])
{
    int bad = 0;
    for (int v = 0; v < 256; v++) {                  /* every byte value */
        if (fpga_port_xfer((uint8_t)v) != (uint8_t)v) bad++;
    }
    for (int i = 0; i < FPGA_IMAGE_BYTES; i++) {     /* the real payload */
        uint8_t b = bitmap_byte(bm, i);
        if (fpga_port_xfer(b) != b) bad++;
    }
    return bad;
}

const char *fpga_status_str(fpga_status_t s)
{
    switch (s) {
    case FPGA_OK:            return "ok";
    case FPGA_ERR_TIMEOUT:   return "timeout waiting for done";
    case FPGA_ERR_BAD_CLASS: return "class out of range";
    }
    return "unknown";
}