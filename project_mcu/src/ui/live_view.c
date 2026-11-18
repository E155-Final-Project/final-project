#include "live_view.h"
#include "config.h"

#include <stddef.h> /* NULL */

#define PANE_W (2 * BITMAP_HEIGHT)

static char_out_function out_fn;
static uint64_t    shown[BITMAP_WORDS];   /* cells already drawn */
static int         last_r = -1, last_c = -1;

static void out_str(const char *s)
{
    while (*s != '\0') {
        out_fn(*s++);
    }
}

static void out_uint(unsigned v)
{
    char buf[6];
    int i = 0;
    do {
        buf[i++] = (char)('0' + v % 10);
        v /= 10;
    } while (v != 0);
    while (i > 0) {
        out_fn(buf[--i]);
    }
}

/* Move the cursor to a 1-based terminal row and column. */
static void cursor_to(unsigned row, unsigned col)
{
    out_str("\x1b[");
    out_uint(row);
    out_fn(';');
    out_uint(col);
    out_fn('H');
}

static void draw_grid(unsigned left, const uint64_t *bm)
{
    for (int r = 0; r < BITMAP_WIDTH; r++) {
        cursor_to(LIVE_TOP + r, left);
        for (int c = 0; c < BITMAP_HEIGHT; c++) {
            out_str((bm != NULL && bitmap_get(bm, r, c)) ? "##" : ". ");
        }
    }
}

/* Raw Trill coordinates -> fixed grid cell. Applies the orientation flags. */
static void map_cell(const stroke_pt_t *p, int *r, int *c)
{
    uint32_t x = p->x, y = p->y;
#if RAST_SWAP_XY
    uint32_t t = x; x = y; y = t;
#endif
    int col = (int)(x * BITMAP_HEIGHT / (TRILL_X_MAX + 1u));
    int row = (int)(y * BITMAP_WIDTH / (TRILL_Y_MAX + 1u));
    if (col >= BITMAP_HEIGHT) col = BITMAP_HEIGHT - 1;   /* out-of-range input */
    if (row >= BITMAP_WIDTH) row = BITMAP_WIDTH - 1;
#if RAST_FLIP_X
    col = BITMAP_HEIGHT - 1 - col;
#endif
#if RAST_FLIP_Y
    row = BITMAP_WIDTH - 1 - row;
#endif
    *r = row;
    *c = col;
}

static void plot(int r, int c)
{
    if (bitmap_get(shown, r, c)) {
        return;                    /* already on screen: send nothing */
    }
    bitmap_set(shown, r, c);
    cursor_to(LIVE_TOP + r, LIVE_LEFT + 2 * c);
    out_str("##");
}

/* Integer Bresenham between two cells, all octants. */
static void line(int r0, int c0, int r1, int c1)
{
    int dc = (c1 > c0) ? c1 - c0 : c0 - c1;
    int dr = (r1 > r0) ? r0 - r1 : r1 - r0;   /* negative */
    int sc = (c0 < c1) ? 1 : -1;
    int sr = (r0 < r1) ? 1 : -1;
    int err = dc + dr;
    for (;;) {
        plot(r0, c0);
        if (r0 == r1 && c0 == c1) {
            break;
        }
        int e2 = 2 * err;
        if (e2 >= dr) { err += dr; c0 += sc; }
        if (e2 <= dc) { err += dc; r0 += sr; }
    }
}

void live_begin(char_out_function out)
{
    out_fn = out;
    bitmap_clear(shown);
    last_r = last_c = -1;
    out_str("\x1b[2J\x1b[?25l");                  /* clear screen, hide cursor */
    cursor_to(1, LIVE_LEFT);
    out_str("live");
    cursor_to(1, LIVE_LEFT + PANE_W + LIVE_GAP);
    out_str("bitmap sent to FPGA");
    draw_grid(LIVE_LEFT, NULL);
    draw_grid(LIVE_LEFT + PANE_W + LIVE_GAP, NULL);
}

void live_point(const stroke_pt_t *p)
{
    if (!p->pen_down) {
        last_r = -1;                              /* lift: don't join strokes */
        return;
    }
    int r, c;
    map_cell(p, &r, &c);
    if (last_r < 0) {
        plot(r, c);
    } else {
        line(last_r, last_c, r, c);
    }
    last_r = r;
    last_c = c;
}

void live_show_result(const uint64_t bm[BITMAP_WORDS], char cls)
{
    draw_grid(LIVE_LEFT + PANE_W + LIVE_GAP, bm);
    cursor_to(LIVE_TOP + BITMAP_WIDTH + 1, LIVE_LEFT);
    out_str("predicted: ");
    out_fn(cls);
}

void live_end(void)
{
    cursor_to(LIVE_TOP + BITMAP_WIDTH + 3, 1);
    out_str("\x1b[?25h");                          /* show cursor */
}