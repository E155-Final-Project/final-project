#include "fpga_link.h"
#include "fpga_port.h"
#include "classes.h"
#include "weights.h"

static volatile int done_flag;

void fpga_link_init(void)
{
    fpga_port_cs(0);  /* release CS */
    done_flag = 0;
}

void fpga_link_done_isr(void)
{
    done_flag = 1;
}

static void begin(uint8_t cmd)
{
    fpga_port_cs(1);  /* pull CS low */
    (void)fpga_port_xfer(cmd);
}

static void end(void)
{
    fpga_port_cs(0);
}

fpga_status_t fpga_load_weights(void)
{
    begin(FPGA_CMD_LOAD_WEIGHTS);
    for (uint32_t k = 0; k < BNN_WEIGHT_WORDS; k++) {
        uint64_t w = bnn_weights[k];
        for (int b = 0; b < 8; b++) {
            (void)fpga_port_xfer((uint8_t)(w >> (8 * b)));   /* LSB byte first */
        }
    }
    end();

    begin(FPGA_CMD_LOAD_THRESH);
    for (uint32_t k = 0; k < BNN_THRESH_COUNT; k++) {
        (void)fpga_port_xfer((uint8_t)(bnn_thresh[k]));
        (void)fpga_port_xfer((uint8_t)(bnn_thresh[k] >> 8));
    }
    end();
    return FPGA_OK;
}

fpga_status_t fpga_classify(const uint64_t bm[BITMAP_WORDS], uint8_t *cls)
{
    /* Clear before sending, so a done pulse from this frame can't be missed
     * and a stale one from an earlier frame can't be mistaken for it. */
    done_flag = 0;

    begin(FPGA_CMD_CLASSIFY);
    for (int i = 0; i < BITMAP_SPI_BYTES; i++) {
        (void)fpga_port_xfer(bitmap_byte(bm, i));
    }
    end();

    uint32_t start = fpga_port_millis();
    while (!done_flag) {
        if ((uint32_t)(fpga_port_millis() - start) > FPGA_DONE_TIMEOUT_MS) {
            return FPGA_ERR_TIMEOUT;
        }
    }

    begin(FPGA_CMD_READ_RESULT);
    uint8_t r = fpga_port_xfer(0x00);    /* dummy byte; the class comes back */
    end();

    if (r == FPGA_RESULT_NONE) {
        return FPGA_ERR_NO_RESULT;
    }
    if (r >= NUM_CLASSES) {
        return FPGA_ERR_BAD_CLASS;
    }
    *cls = r;
    return FPGA_OK;
}

fpga_status_t fpga_clear(void)
{
    begin(FPGA_CMD_CLEAR);
    end();
    return FPGA_OK;
}

int fpga_link_loopback_test(const uint64_t bm[BITMAP_WORDS])
{
    int bad = 0;
    fpga_port_cs(1);
    for (int v = 0; v < 256; v++) {                  /* every byte value */
        if (fpga_port_xfer((uint8_t)v) != (uint8_t)v) bad++;
    }
    for (int i = 0; i < BITMAP_SPI_BYTES; i++) {     /* the real payload */
        uint8_t b = bitmap_byte(bm, i);
        if (fpga_port_xfer(b) != b) bad++;
    }
    fpga_port_cs(0);
    return bad;
}

const char *fpga_status_str(fpga_status_t s)
{
    switch (s) {
    case FPGA_OK:            return "ok";
    case FPGA_ERR_TIMEOUT:   return "timeout waiting for done";
    case FPGA_ERR_NO_RESULT: return "FPGA has no result (0xFF)";
    case FPGA_ERR_BAD_CLASS: return "class out of range";
    }
    return "unknown";
}