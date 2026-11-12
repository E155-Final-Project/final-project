#include <stdio.h>
#include <stdint.h>

#include "bitmap.h"
#include "classes.h"
#include "fpga_link.h"
#include "fpga_port_fake.h"
#include "golden.h"
#include "mem_io.h"
#include "rasterize.h"
#include "strokes.h"

/* Test 6: the SPI protocol against a simulated FPGA. */

#define SMOKE_IMAGES 16

static int failures;

static void check(int ok, const char *what)
{
    printf("  %-56s %s\n", what, ok ? "ok" : "FAIL");
    if (!ok) failures++;
}

int main(void)
{
    static uint64_t smoke[SMOKE_IMAGES * BITMAP_WORDS];
    static uint64_t expected[SMOKE_IMAGES];
    if (mem_read("../BNN/vectors/smoke/inputs.mem", smoke, SMOKE_IMAGES * BITMAP_WORDS) != 0) return 1;
    if (mem_read("../BNN/vectors/smoke/expected.mem", expected, SMOKE_IMAGES) != 0) return 1;
    if (golden_load("../BNN/mem") != 0) return 1;

    fake_reset();
    fpga_link_init();

    printf("weight load:\n");
    check(fpga_load_weights() == FPGA_OK, "fpga_load_weights returns ok");
    check(fake_weights_ok, "FPGA received all 121,792 weight bytes intact");
    check(fake_thresh_ok, "FPGA received all 3,072 threshold bytes intact");

    printf("classify the 16 smoke images over the link:\n");
    int wire_ok = 1, cls_ok = 1;
    for (int n = 0; n < SMOKE_IMAGES; n++) {
        const uint64_t *bm = &smoke[n * BITMAP_WORDS];
        uint8_t cls = 0xEE;
        fpga_status_t s = fpga_classify(bm, &cls);
        for (int k = 0; k < BITMAP_WORDS; k++) {
            if (fake_last_bitmap[k] != bm[k]) wire_ok = 0;
        }
        if (s != FPGA_OK || cls != expected[n]) cls_ok = 0;
    }
    check(wire_ok, "FPGA reassembles every bitmap word-exact");
    check(cls_ok, "every class read back matches expected.mem");

    printf("rasterized strokes end to end:\n");
    for (int i = 0; i < N_TEST_STROKES; i++) {
        const test_stroke_t *st = &TEST_STROKES[i];
        uint64_t bm[BITMAP_WORDS];
        uint8_t cls = 0xEE;
        rasterize(st->pts, st->n, bm, NULL);
        fpga_status_t s = fpga_classify(bm, &cls);
        char what[64];
        snprintf(what, sizeof what, "stroke '%s' -> FPGA -> '%c'", st->name, class_char(cls));
        check(s == FPGA_OK && cls == st->expected_class, what);
    }

    printf("error paths:\n");
    uint8_t cls = 0xEE;
    fake_mode = FAKE_NEVER_DONE;
    check(fpga_classify(&smoke[0], &cls) == FPGA_ERR_TIMEOUT, "no done: FPGA_ERR_TIMEOUT");
    fake_mode = FAKE_NO_RESULT;
    check(fpga_classify(&smoke[0], &cls) == FPGA_ERR_NO_RESULT, "result 0xFF: FPGA_ERR_NO_RESULT");
    fake_mode = FAKE_NORMAL;
    int clears = fake_clears;
    check(fpga_clear() == FPGA_OK && fake_clears == clears + 1, "fpga_clear sends one CLEAR frame");
    check(fake_bad_frames == 0, "no malformed frames in the whole run");

    printf("\ntest 6 (FPGA link vs simulated FPGA): %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}