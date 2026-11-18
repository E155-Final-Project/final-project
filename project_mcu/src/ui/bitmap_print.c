#include "bitmap_print.h"

static void out_ink(char_out_function out) {
    out((char)0xE2);
    out((char)0x96);
    out((char)0x88);
}

static void out_str(const char *s, char_out_function out) {
    while (*s != '\0') {
        out(*s++);
    }
}

void bitmap_print(const uint64_t bm[BITMAP_WORDS], char_out_function out) {
    for (int row = 0; row < BITMAP_HEIGHT; row++) {
        for (int col = 0; col < BITMAP_WIDTH; col++) {
            if (bitmap_get(bm, row, col)) {
                out_ink(out);
            } else {
                out('.');
            }
        }
        out_str("\r\n", out);
    }
}


