#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>

#define BITMAP_WIDTH 28
#define BITMAP_HEIGHT 28
#define BITMAP_PIXELS (BITMAP_WIDTH * BITMAP_HEIGHT)         /* 784 */
#define BITMAP_WORDS ((BITMAP_PIXELS + 63) / 64)    /* 13 */
#define BITMAP_BYTES (BITMAP_WORDS * 8)             /* 104 */
#define BITMAP_SPI_BYTES  (BITMAP_PIXELS / 8)   /* 98: pixels 0..783, no padding */

/* Byte i of the packed bitmap: pixels 8i..8i+7, pixel 8i in bit 0.
 * Sent with SPI LSBFIRST = 1, the wire carries pixel 0 first, pixel 783 last.
 * Computed by shifting so it doesn't depend on the CPU's byte order. */
static inline uint8_t bitmap_byte(const uint64_t bm[BITMAP_WORDS], int i)
{
    return (uint8_t)(bm[i / 8] >> (8 * (i % 8)));
}

/* Pixel p = row*28 + col, with row 0 located at the top
 * Bit (p % 64) of word (p / 64), matching pack_words() in bnn_emnist.py
 * Ink = 1. Bits 787..831 (top 48 pixels) are used as padding and should be 0 . */

 static inline int bitmap_get(const uint64_t bm[BITMAP_WORDS], int row, int col) {
    int p = row * BITMAP_WIDTH + col;
    return (int)(bm[p / 64] >> (p % 64)) & 1u;
 }

 static inline void bitmap_set(uint64_t bm[BITMAP_WORDS], int row, int col) {
    int p = row * BITMAP_WIDTH + col;
    bm[p / 64] |= (uint64_t)1 << (p % 64);
 }

 static inline void bitmap_clear(uint64_t bm[BITMAP_WORDS]) {
    for (int k = 0; k < BITMAP_WORDS; k++) {
        bm[k] = 0;
    }
 }
#endif // BITMAP_H