#include "mem_io.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A 64 bit word = 16 hex digits plus "\r\n" and the terminator */
#define MEM_LINE_MAX 64

/* True if s contains only whitespace */
static int is_blank(const char *s) {
    while (*s != '\0') {
        if (!isspace((unsigned char)*s)) {
            return 0;
        }
        s++;
    }
    return 1;
}

int mem_read(const char *path, uint64_t *out, size_t expected) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        fprintf(stderr, "mem_read: failed to open  %s\n", path);
        return -1;
    }
    char line[MEM_LINE_MAX];
    size_t count = 0;
    unsigned lineno = 0;
    int rc = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        lineno++;

        if (strchr(line, '\n') == NULL && !feof(f)){
            fprintf(stderr, "mem_read: %s:%u: line too long\n", path, lineno);
            rc = -1;
            break;
        }
        if (is_blank(line)) {
            continue;
        }

        const char *p = line;
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (!isxdigit((unsigned char)*p)) {
            fprintf(stderr, "mem_read: %s:%u: expected hex value\n", path, lineno);
            rc = -1;
            break;
        }

        errno = 0;
        char *end;
        unsigned long long val = strtoull(p, &end, 16);
        if (errno == ERANGE) {
            fprintf(stderr, "mem_read: %s:%u: bad hex value\n", path, lineno);
            rc = -1;
            break;
        }

        if (count >= expected) {
            fprintf(stderr, "mem_read: %s: more than %zu values\n", path, expected);
            rc = -1;
            break;
        }
        out[count++] = (uint64_t)val;
    }

if (rc == 0 && ferror(f)) {
    fprintf(stderr, "mem_read: %s: read error\n", path);
    rc = -1;
}

if(rc == 0 && count != expected) {
    fprintf(stderr, "mem_read: %s: got %zu values, expected %zu\n", path, count, expected);
    rc = -1;
}
fclose(f);
return rc;
}