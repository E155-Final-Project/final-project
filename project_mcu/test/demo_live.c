/* demo_live.c -- laptop preview of the MCU's USART2 live view.
 *
 * Replays the test strokes the way the TPS65 will deliver them, through the
 * same live_view.c the MCU runs (here its output goes to stdout instead of
 * USART2). When a character ends, it is rasterized with the firmware's
 * rasterize() and classified with the C golden model, standing in for FPGA_A.
 *
 *   make demo            normal speed
 *   ./build/demo_live 4  4x faster;  ./build/demo_live 0  no delays
 *
 * Terminal: at least 70 columns x 36 rows, UTF-8 not required. */

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "bitmap.h"
#include "classes.h"
#include "config.h"
#include "golden.h"
#include "live_view.h"
#include "rasterize.h"
#include "strokes.h"

static double speed = 1.0;          /* 0 = no delays */

static void host_out(char c) { fputc(c, stdout); }

static void pause_ms(unsigned ms)
{
    fflush(stdout);
    if (speed <= 0.0) return;
    double s = ms / 1000.0 / speed;
    struct timespec t = { (time_t)s, (long)((s - (time_t)s) * 1e9) };
    nanosleep(&t, NULL);
}

/* Text below the two panes, at a fixed row, cleared to end of line. */
static void status(int line, const char *text)
{
    printf("\x1b[%d;%dH\x1b[K%s", LIVE_TOP + BITMAP_WIDTH + 2 + line, LIVE_LEFT, text);
}

/* Feed one stroke list as 100 Hz touch samples: interpolate between the
 * recorded points so the trace grows the way a finger would draw it. */
static void replay(const test_stroke_t *t)
{
    const stroke_pt_t *prev = NULL;
    for (uint16_t i = 0; i < t->n; i++) {
        const stroke_pt_t *p = &t->pts[i];
        if (!p->pen_down) {                          /* finger lifted between strokes */
            live_point(p);
            status(0, "finger up ... next stroke");
            pause_ms(250);
            status(0, "drawing ...");
            prev = NULL;
            continue;
        }
        if (prev) {
            for (int k = 1; k <= 4; k++) {           /* 4 samples per segment */
                stroke_pt_t s = {
                    (uint16_t)(prev->x + (p->x - prev->x) * k / 4),
                    (uint16_t)(prev->y + (p->y - prev->y) * k / 4), 1 };
                live_point(&s);
                pause_ms(10);
            }
        } else {
            live_point(p);
            pause_ms(10);
        }
        prev = p;
    }
    stroke_pt_t up = {0, 0, 0};
    live_point(&up);
}

int main(int argc, char **argv)
{
    if (argc > 1) speed = atof(argv[1]);
    if (golden_load("../BNN/mem") != 0) return 1;

    for (int i = 0; i < N_TEST_STROKES; i++) {
        const test_stroke_t *t = &TEST_STROKES[i];
        char line[96];

        live_begin(host_out);
        status(0, "drawing ...");
        replay(t);

        status(0, "finger up: waiting 500 ms for more strokes ...");
        pause_ms(500);

        uint64_t bm[BITMAP_WORDS];
        rast_stats_t st;
        rasterize(t->pts, t->n, bm, &st);
        uint16_t scores[NUM_CLASSES];
        int cls = golden_infer(bm, scores);          /* on the board: FPGA_A over SPI */

        live_show_result(bm, class_char((uint8_t)cls));

        /* top 3 classes by score */
        int top[3] = {-1, -1, -1};
        for (int k = 0; k < 3; k++)
            for (int c = 0; c < NUM_CLASSES; c++)
                if (c != top[0] && c != top[1] &&
                    (top[k] < 0 || scores[c] > scores[top[k]])) top[k] = c;

        snprintf(line, sizeof line, "strokes %u   points %u   box %ux%u   ink %u px",
                 st.n_strokes, st.n_points, st.box_rows, st.box_cols, st.ink);
        status(0, line);
        snprintf(line, sizeof line, "top 3: %c %u   %c %u   %c %u      (expected '%c')",
                 class_char((uint8_t)top[0]), scores[top[0]],
                 class_char((uint8_t)top[1]), scores[top[1]],
                 class_char((uint8_t)top[2]), scores[top[2]],
                 class_char(t->expected_class));
        status(1, line);
        status(2, "laptop preview: same live_view.c as the MCU; C golden model stands in for FPGA_A");
        pause_ms(3500);
    }
    live_end();
    return 0;
}