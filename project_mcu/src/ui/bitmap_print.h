#ifndef BITMAP_PRINT_H
#define BITMAP_PRINT_H

#include <stdint.h>
#include "bitmap.h"

/* Character output function: fputc on the laptop (USART2 on STM32). */
typedef void (*char_out_function)(char c);

/* Print the 28x28 bitmap, onw row per line, full block for ink, '.' for background. */
void bitmap_print(const uint64_t bm[BITMAP_WORDS], char_out_function out);

#endif // BITMAP_PRINT_H