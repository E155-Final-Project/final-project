#include "fpga_port_fake.h"
#include "fpga_port.h"
#include "fpga_link.h"
#include "golden.h"
#include "weights.h"

fake_mode_t fake_mode;
int      fake_frames, fake_bad_frames, fake_clears, fake_weights_ok, fake_thresh_ok;
uint64_t fake_last_bitmap[BITMAP_WORDS];

static uint32_t clock_ms;
static int      cs_active;
static uint32_t nbytes;           /* bytes in the current frame, command included */
static uint8_t  cmd;
static uint8_t  result = FPGA_RESULT_NONE;
static uint8_t  next_miso;        /* what goes out on MISO during the next byte */

/* Bitmap reassembly, as the FPGA slave would do it: each bit shifts into the
 * top of a 64-bit register; every 64 bits the register becomes the next word. */
static uint64_t shreg;
static int      shbits, word_idx;

/* Running comparison of a LOAD payload against the flash arrays. */
static uint32_t load_mismatch;

void fake_reset(void)
{
    fake_mode = FAKE_NORMAL;
    fake_frames = fake_bad_frames = fake_clears = 0;
    fake_weights_ok = fake_thresh_ok = 0;
    clock_ms = 0;
    cs_active = 0;
    result = FPGA_RESULT_NONE;
}

uint32_t fpga_port_millis(void)
{
    return clock_ms++;            /* time advances 1 ms per call */
}

static void frame_start(void)
{
    nbytes = 0;
    shreg = 0;
    shbits = 0;
    word_idx = 0;
    load_mismatch = 0;
}

static void frame_end(void)
{
    fake_frames++;
    uint32_t payload = nbytes ? nbytes - 1 : 0;
    switch (nbytes ? cmd : 0) {
    case FPGA_CMD_CLASSIFY:
        if (payload != BITMAP_SPI_BYTES) { fake_bad_frames++; return; }
        /* Last word got only 16 bits, now in bits 48..63: shift down, zero-pad. */
        fake_last_bitmap[word_idx] = shreg >> (64 - shbits);
        {
            uint16_t scores[NUM_CLASSES];
            result = (uint8_t)golden_infer(fake_last_bitmap, scores);
        }
        if (fake_mode == FAKE_NO_RESULT) result = FPGA_RESULT_NONE;
        if (fake_mode != FAKE_NEVER_DONE) fpga_link_done_isr();
        break;
    case FPGA_CMD_READ_RESULT:
        if (payload != 1) fake_bad_frames++;
        break;
    case FPGA_CMD_CLEAR:
        if (payload != 0) { fake_bad_frames++; return; }
        fake_clears++;
        result = FPGA_RESULT_NONE;
        break;
    case FPGA_CMD_LOAD_WEIGHTS:
        fake_weights_ok = (payload == BNN_WEIGHT_BYTES) && load_mismatch == 0;
        if (!fake_weights_ok) fake_bad_frames++;
        break;
    case FPGA_CMD_LOAD_THRESH:
        fake_thresh_ok = (payload == BNN_THRESH_BYTES) && load_mismatch == 0;
        if (!fake_thresh_ok) fake_bad_frames++;
        break;
    default:
        fake_bad_frames++;
    }
}

void fpga_port_cs(int active)
{
    if (active && !cs_active) frame_start();
    if (!active && cs_active) frame_end();
    cs_active = active;
}

uint8_t fpga_port_xfer(uint8_t out)
{
    uint8_t in = next_miso;
    next_miso = 0;
    if (!cs_active) return in;

    if (nbytes == 0) {
        cmd = out;
        if (cmd == FPGA_CMD_READ_RESULT) next_miso = result;   /* ready for the dummy byte */
        if (cmd == FPGA_CMD_CLASSIFY) {                        /* new bitmap incoming */
            for (int k = 0; k < BITMAP_WORDS; k++) fake_last_bitmap[k] = 0;
        }
    } else {
        uint32_t i = nbytes - 1;          /* payload byte index */
        switch (cmd) {
        case FPGA_CMD_CLASSIFY:
            for (int b = 0; b < 8; b++) {                /* LSB first on the wire */
                shreg = (shreg >> 1) | ((uint64_t)((out >> b) & 1u) << 63);
                if (++shbits == 64) {
                    if (word_idx < BITMAP_WORDS) fake_last_bitmap[word_idx] = shreg;
                    word_idx++;
                    shreg = 0;
                    shbits = 0;
                }
            }
            break;
        case FPGA_CMD_LOAD_WEIGHTS:
            if (i / 8 < BNN_WEIGHT_WORDS &&
                (uint8_t)(bnn_weights[i / 8] >> (8 * (i % 8))) != out) load_mismatch++;
            break;
        case FPGA_CMD_LOAD_THRESH:
            if (i / 2 < BNN_THRESH_COUNT &&
                (uint8_t)(bnn_thresh[i / 2] >> (8 * (i % 2))) != out) load_mismatch++;
            break;
        default:
            break;
        }
    }
    nbytes++;
    return in;
}