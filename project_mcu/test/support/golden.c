#include "golden.h"
#include "mem_io.h"

#include <stdio.h>

#define L0_IN_WORDS 13
#define HID         512
#define HID_WORDS (HID / 64)

static uint64_t w0[HID * L0_IN_WORDS];
static uint64_t w1[HID * HID_WORDS];
static uint64_t w2[HID * HID_WORDS];
static uint64_t w3[NUM_CLASSES * HID_WORDS];
static uint64_t t0[HID], t1[HID], t2[HID];

static int load(const char *dir, const char *name, uint64_t *out, size_t n)
{
    char path[512];
    snprintf(path, sizeof path, "%s/%s", dir, name);
    return mem_read(path, out, n);
}

int golden_load(const char *mem_dir)
{
    int rc = 0;
    rc |= load(mem_dir, "l0_weights.mem", w0, sizeof w0 / sizeof w0[0]);
    rc |= load(mem_dir, "l1_weights.mem", w1, sizeof w1 / sizeof w1[0]);
    rc |= load(mem_dir, "l2_weights.mem", w2, sizeof w2 / sizeof w2[0]);
    rc |= load(mem_dir, "l3_weights.mem", w3, sizeof w3 / sizeof w3[0]);
    rc |= load(mem_dir, "l0_thresh.mem", t0, HID);
    rc |= load(mem_dir, "l1_thresh.mem", t1, HID);
    rc |= load(mem_dir, "l2_thresh.mem", t2, HID);
    return rc ? -1 : 0;
}
/* popcount(XNOR(in, weights)) summed over the neuron's words.
 * Pad bits: input 0, weight 1, so XNOR is 0 there and adds nothing. */
static uint16_t neuron(const uint64_t *in, const uint64_t *w, int words)
{
    uint16_t p = 0;
    for (int k = 0; k < words; k++) {
        p += (uint16_t)__builtin_popcountll(~(in[k] ^ w[k]));
    }
    return p;
}

/* Hidden layer: neuron m fires when popcount >= threshold; bit m of out. */
static void hidden(const uint64_t *in, int in_words, const uint64_t *w,
                   const uint64_t *th, uint64_t out[HID_WORDS])
{
    for (int k = 0; k < HID_WORDS; k++) {
        out[k] = 0;
    }
    for (int m = 0; m < HID; m++) {
        if (neuron(in, &w[m * in_words], in_words) >= th[m]) {
            out[m / 64] |= (uint64_t)1 << (m % 64);
        }
    }
}

int golden_infer(const uint64_t in[BITMAP_WORDS], uint16_t scores[NUM_CLASSES])
{
    uint64_t a1[HID_WORDS], a2[HID_WORDS], a3[HID_WORDS];
    hidden(in, L0_IN_WORDS, w0, t0, a1);
    hidden(a1, HID_WORDS, w1, t1, a2);
    hidden(a2, HID_WORDS, w2, t2, a3);

    /* Output layer: no threshold. Argmax with strict '>', lowest index wins ties. */
    int best = 0;
    for (int c = 0; c < NUM_CLASSES; c++) {
        scores[c] = neuron(a3, &w3[c * HID_WORDS], HID_WORDS);
        if (scores[c] > scores[best]) {
            best = c;
        }
    }
    return best;
}