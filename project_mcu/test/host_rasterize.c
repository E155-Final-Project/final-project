#include <stdio.h>
#include <stdint.h>

#include "bitmap.h"
#include "bitmap_print.h"
#include "mem_io.h"
#include "rasterize.h"
#include "strokes.h" 
#include "golden.h"
#include "classes.h"


#define SMOKE_IMAGES 16

static void host_out(char c)
{
    fputc(c, stdout);
}

/* Returns -1 if equal, else the first differing byte index (all 104 bytes,
 * so the 48 pad bits are checked too). */
static int bitmap_first_diff(const uint64_t a[BITMAP_WORDS], const uint64_t b[BITMAP_WORDS])
{
    for (int i = 0; i < BITMAP_BYTES; i++) {
        if (bitmap_byte(a, i) != bitmap_byte(b, i)) {
            return i;
        }
    }
    return -1;
}

/* Test 1: unpack each smoke image pixel by pixel, repack it, compare to the file. */
static int test_roundtrip(const uint64_t *smoke)
{
    int failures = 0;
    for (int n = 0; n < SMOKE_IMAGES; n++) {
        const uint64_t *orig = &smoke[n * BITMAP_WORDS];
        uint64_t repacked[BITMAP_WORDS];
        bitmap_clear(repacked);

        for (int row = 0; row < BITMAP_WIDTH; row++) {
            for (int col = 0; col < BITMAP_HEIGHT; col++) {
                if (bitmap_get(orig, row, col)) {
                    bitmap_set(repacked, row, col);
                }
            }
        }

        int diff = bitmap_first_diff(orig, repacked);
        if (diff < 0) {
            printf("image %2d: MATCH\n", n);
        } else {
            printf("image %2d: MISMATCH at byte %d\n", n, diff);
            failures++;
        }
    }
    return failures;
}

/* Number of pixels that differ between two bitmaps. */
static int bitmap_diff_pixels(const uint64_t a[BITMAP_WORDS], const uint64_t b[BITMAP_WORDS])
{
    int n = 0;
    for (int k = 0; k < BITMAP_WORDS; k++) {
        n += __builtin_popcountll(a[k] ^ b[k]);
    }
    return n;
}

static void put_pixel(int on)
{
    if (on) {
        fputc((char)0xE2, stdout);
        fputc((char)0x96, stdout);
        fputc((char)0x88, stdout);
    } else {
        fputc('.', stdout);
    }
}

/* Two bitmaps side by side. */
static void print_pair(const char *la, const uint64_t a[BITMAP_WORDS],
                       const char *lb, const uint64_t b[BITMAP_WORDS])
{
    printf("%-30s %s\n", la, lb);
    for (int r = 0; r < BITMAP_WIDTH; r++) {
        for (int c = 0; c < BITMAP_HEIGHT; c++) put_pixel(bitmap_get(a, r, c));
        fputs("   ", stdout);
        for (int c = 0; c < BITMAP_HEIGHT; c++) put_pixel(bitmap_get(b, r, c));
        fputc('\n', stdout);
    }
}

/* Test 2: rasterize the hardcoded strokes; compare the 'a' to smoke image 0.
 * A mismatch is expected. The numbers are for tuning, not pass/fail. */
static int test_rasterize(const uint64_t *smoke)
{
    int failures = 0;
    for (int i = 0; i < N_TEST_STROKES; i++) {
        const test_stroke_t *s = &TEST_STROKES[i];
        uint64_t bm[BITMAP_WORDS];
        rast_stats_t st;
        if (rasterize(s->pts, s->n, bm, &st) != 0) {
            printf("stroke '%s': rasterize failed\n", s->name);
            failures++;
            continue;
        }
        if (bm[BITMAP_WORDS - 1] >> 16) {
            printf("stroke '%s': padding bits set\n", s->name);
            failures++;
        }
        printf("\nstroke '%s': %u points, %u strokes, box %u x %u, %u ink pixels\n",
               s->name, st.n_points, st.n_strokes, st.box_rows, st.box_cols, st.ink);
        if (i == 0) {
            print_pair("rasterized 'a'", bm, "smoke image 0", &smoke[0]);
            int d = bitmap_first_diff(bm, &smoke[0]);
            if (d < 0) {
                printf("vs smoke image 0: MATCH\n");
            } else {
                printf("vs smoke image 0: MISMATCH at byte %d, %d pixels differ (expected)\n",
                       d, bitmap_diff_pixels(bm, &smoke[0]));
            }
        } else {
            bitmap_print(bm, host_out);
        }
    }
    return failures;
}

/* Print the k highest-scoring classes. */
static void print_top(const uint16_t scores[NUM_CLASSES], int k)
{
    int used[NUM_CLASSES] = {0};
    for (int i = 0; i < k; i++) {
        int best = -1;
        for (int c = 0; c < NUM_CLASSES; c++) {
            if (!used[c] && (best < 0 || scores[c] > scores[best])) {
                best = c;
            }
        }
        used[best] = 1;
        printf("  %c %3u\n", class_char((uint8_t)best), scores[best]);
    }
}

/* Test 3a: the C golden model must reproduce the Python golden model exactly,
 * on both the predicted class and all 47 output popcounts. */
static int test_golden_smoke(const uint64_t *smoke, const char *vec_dir)
{
    static uint64_t expected[SMOKE_IMAGES];
    static uint64_t l3_pc[SMOKE_IMAGES * NUM_CLASSES];
    char path[512];
    snprintf(path, sizeof path, "%s/expected.mem", vec_dir);
    if (mem_read(path, expected, SMOKE_IMAGES) != 0) return 1;
    snprintf(path, sizeof path, "%s/l3_popcount.mem", vec_dir);
    if (mem_read(path, l3_pc, SMOKE_IMAGES * NUM_CLASSES) != 0) return 1;

    int failures = 0;
    for (int n = 0; n < SMOKE_IMAGES; n++) {
        uint16_t scores[NUM_CLASSES];
        int cls = golden_infer(&smoke[n * BITMAP_WORDS], scores);
        int bad_scores = 0;
        for (int c = 0; c < NUM_CLASSES; c++) {
            if (scores[c] != l3_pc[n * NUM_CLASSES + c]) bad_scores++;
        }
        int ok = (cls == (int)expected[n]) && bad_scores == 0;
        printf("image %2d: predicted %c, expected %c, %d scores differ: %s\n",
               n, class_char((uint8_t)cls), class_char((uint8_t)expected[n]),
               bad_scores, ok ? "MATCH" : "MISMATCH");
        if (!ok) failures++;
    }
    return failures;
}

/* Test 3b: classify the rasterized test strokes. */
static int test_golden_strokes(void)
{
    int failures = 0;
    for (int i = 0; i < N_TEST_STROKES; i++) {
        const test_stroke_t *s = &TEST_STROKES[i];
        uint64_t bm[BITMAP_WORDS];
        uint16_t scores[NUM_CLASSES];
        if (rasterize(s->pts, s->n, bm, NULL) != 0) { failures++; continue; }
        int cls = golden_infer(bm, scores);
        int ok = (cls == s->expected_class);
        printf("stroke '%s': predicted %c: %s\n", s->name,
               class_char((uint8_t)cls), ok ? "PASS" : "FAIL");
        print_top(scores, 5);
        if (!ok) failures++;
    }
    return failures;
}

/* Test 4: the 98-byte SPI stream, read LSB first, must be pixels 0..783 in order. */
static int test_spi_order(const uint64_t *smoke)
{
    int failures = 0;
    for (int n = 0; n < SMOKE_IMAGES; n++) {
        const uint64_t *bm = &smoke[n * BITMAP_WORDS];
        int bad = -1;
        for (int bit = 0; bit < BITMAP_PIXELS && bad < 0; bit++) {
            int wire = (bitmap_byte(bm, bit / 8) >> (bit % 8)) & 1;   /* LSB first */
            if (wire != bitmap_get(bm, bit / BITMAP_HEIGHT, bit % BITMAP_HEIGHT)) {
                bad = bit;
            }
        }
        if (bad >= 0) {
            printf("image %2d: SPI bit %d out of order\n", n, bad);
            failures++;
        }
    }
    return failures;
}

int main(int argc, char **argv)
{
    const char *path = (argc > 1) ? argv[1] : "../BNN/vectors/smoke/inputs.mem";
    static uint64_t smoke[SMOKE_IMAGES * BITMAP_WORDS];

    if (mem_read(path, smoke, SMOKE_IMAGES * BITMAP_WORDS) != 0) {
        return 1;
    }

    printf("smoke image 0 (label 36, 'a'):\n");
    bitmap_print(&smoke[0], host_out);
    printf("\n");

    int failures = test_roundtrip(smoke);
    printf("\ntest 1 (round trip): %s\n", failures ? "FAIL" : "PASS");

    int f2 = test_rasterize(smoke);
    printf("\ntest 2 (rasterize): %s\n", f2 ? "FAIL" : "ran, see diff above");
    
    if (golden_load("../BNN/mem") != 0) return 1;
    int f3a = test_golden_smoke(smoke, "../BNN/vectors/smoke");
    printf("\ntest 3a (golden model vs Python): %s\n\n", f3a ? "FAIL" : "PASS");
    int f3b = test_golden_strokes();
    printf("\ntest 3b (classify strokes): %s\n", f3b ? "FAIL" : "PASS");
    int f4 = test_spi_order(smoke);
    printf("\ntest 4 (SPI order): %s\n", f4 ? "FAIL" : "PASS");
    return (failures || f2 || f3a || f3b || f4) ? 1 : 0;
}

