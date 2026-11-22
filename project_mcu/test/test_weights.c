#include <stdio.h>
#include <stdint.h>

#include "mem_io.h"
#include "weights.h"

/* Test 5: the generated arrays must equal the .mem files, value for value,
 * and the CRC in weights.h must match the data. */

static uint32_t crc32_update(uint32_t crc, uint8_t b)
{
    crc ^= b;
    for (int k = 0; k < 8; k++) {
        crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return crc;
}

int main(void)
{
    static const struct { const char *file; uint32_t start, count; } W[] = {
        {"../BNN/mem/l0_weights.mem", BNN_L0_WORD_START, BNN_L1_WORD_START - BNN_L0_WORD_START},
        {"../BNN/mem/l1_weights.mem", BNN_L1_WORD_START, BNN_L2_WORD_START - BNN_L1_WORD_START},
        {"../BNN/mem/l2_weights.mem", BNN_L2_WORD_START, BNN_L3_WORD_START - BNN_L2_WORD_START},
        {"../BNN/mem/l3_weights.mem", BNN_L3_WORD_START, BNN_WEIGHT_WORDS - BNN_L3_WORD_START},
    };
    static const struct { const char *file; uint32_t start; } T[] = {
        {"../BNN/mem/l0_thresh.mem", BNN_L0_THRESH_START},
        {"../BNN/mem/l1_thresh.mem", BNN_L1_THRESH_START},
        {"../BNN/mem/l2_thresh.mem", BNN_L2_THRESH_START},
    };
    static uint64_t buf[BNN_WEIGHT_WORDS];
    int failures = 0;

    for (int i = 0; i < 4; i++) {
        if (mem_read(W[i].file, buf, W[i].count) != 0) return 1;
        uint32_t bad = 0;
        for (uint32_t k = 0; k < W[i].count; k++) {
            if (buf[k] != bnn_weights[W[i].start + k]) bad++;
        }
        printf("layer %d weights: %5u words, %u differ\n", i, W[i].count, bad);
        if (bad) failures++;
    }
    for (int i = 0; i < 3; i++) {
        if (mem_read(T[i].file, buf, 512) != 0) return 1;
        uint32_t bad = 0;
        for (uint32_t k = 0; k < 512; k++) {
            if (buf[k] != bnn_thresh[T[i].start + k]) bad++;
        }
        printf("layer %d thresholds: 512 values, %u differ\n", i, bad);
        if (bad) failures++;
    }

    /* CRC over the SPI payload: each value LSB byte first. */
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t k = 0; k < BNN_WEIGHT_WORDS; k++)
        for (int b = 0; b < 8; b++) crc = crc32_update(crc, (uint8_t)(bnn_weights[k] >> (8 * b)));
    for (uint32_t k = 0; k < BNN_THRESH_COUNT; k++)
        for (int b = 0; b < 2; b++) crc = crc32_update(crc, (uint8_t)(bnn_thresh[k] >> (8 * b)));
    crc ^= 0xFFFFFFFFu;
    printf("payload CRC-32: 0x%08X, header says 0x%08X\n", crc, BNN_PAYLOAD_CRC32);
    if (crc != BNN_PAYLOAD_CRC32) failures++;

    printf("\ntest 5 (weights.c vs .mem): %s\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}