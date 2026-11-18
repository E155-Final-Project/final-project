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
#include "weights.h"

/* Test 6: fpga_link against a bit-level model of Pierce's BNN_spi / BNN_top. */

#define SMOKE_IMAGES 16

static int failures;

static void check(int ok, const char *what)
{
    printf("  %-58s %s\n", what, ok ? "ok" : "FAIL");
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
    check(fake_weight_words == BNN_WEIGHT_WORDS, "FPGA wrote exactly 15,224 weight words");
    check(fake_weight_mismatch == 0, "every weight word matches bnn_weights[]");

    printf("classify the 16 smoke images:\n");
    int words_ok = 1, image_ok = 1, cls_ok = 1;
    for (int n = 0; n < SMOKE_IMAGES; n++) {
        const uint64_t *bm = &smoke[n * BITMAP_WORDS];
        uint8_t cls = 0xEE;
        fpga_status_t s = fpga_classify(bm, &cls);
        if (fake_image_words != BITMAP_WORDS) words_ok = 0;
        for (int k = 0; k < BITMAP_WORDS; k++) {
            if (fake_image[k] != bm[k]) image_ok = 0;
        }
        if (s != FPGA_OK || cls != expected[n]) cls_ok = 0;
    }
    check(words_ok, "each image arrives as exactly 13 words");
    check(image_ok, "the image EBR holds every bitmap word-exact (pad = 0)");
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

    printf("protocol timing:\n");
    check(fake_partial_words == 0, "no transfer ends mid-word");
    check(fake_gap_violations == 0, "every load/load_weights edge >= 1 us from SCK");
    check(fake_runs == 1 + SMOKE_IMAGES + N_TEST_STROKES,
          "core started once per image (+1 after the weight load)");

    printf("error paths and clear:\n");
    uint8_t cls = 0xEE;
    fake_mode = FAKE_NEVER_DONE;
    check(fpga_classify(&smoke[0], &cls) == FPGA_ERR_TIMEOUT, "no done: FPGA_ERR_TIMEOUT");
    fake_mode = FAKE_BAD_CLASS;
    check(fpga_classify(&smoke[0], &cls) == FPGA_ERR_BAD_CLASS, "class 0x7F: FPGA_ERR_BAD_CLASS");
    fake_mode = FAKE_NORMAL;
    check(fpga_classify(&smoke[0], &cls) == FPGA_OK && cls == expected[0],
          "recovers on the next classify");
    check(fpga_clear() == FPGA_OK && fake_clear_pulses == 1, "fpga_clear: one pulse >= 10 us");

    printf("\ntest 6 (FPGA link vs BNN_spi model): %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}