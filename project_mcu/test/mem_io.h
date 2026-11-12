#ifndef MEM_IO_H
#define MEM_IO_H

#include <stddef.h>
#include <stdint.h>

/* Read a $readmemh-style file: one hex value per line
 * Reads exactly 'expected' values into the output array.
 * Returns 0 on success, -1 on failure.
 */
int mem_read(const char *path, uint64_t *out, size_t expected);

#endif // MEM_IO_H