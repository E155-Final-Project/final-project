#define _POSIX_C_SOURCE 199309L   /* nanosleep under -std=c11 */
#include <stdio.h>
#include <time.h>

#include "live_view.h"
#include "strokes.h"

static void host_out(char c)
{
    fputc(c, stdout);
}

int main(void)
{
    const test_stroke_t *s = &TEST_STROKES[0];          /* the 'a' */
    struct timespec dt = {0, 40 * 1000 * 1000};         /* 40 ms per point */

    live_begin(host_out);
    for (uint16_t i = 0; i < s->n; i++) {
        live_point(&s->pts[i]);
        fflush(stdout);           /* stdout is buffered; force each update out */
        nanosleep(&dt, NULL);
    }
    live_end();
    return 0;
}