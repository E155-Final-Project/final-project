#include "classes.h"

static const char CLASS_CHARS[NUM_CLASSES + 1] = 
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabdefghnqrt";

char class_char(uint8_t idx)
{
    return (idx < NUM_CLASSES) ? CLASS_CHARS[idx] : '?';
}