#include "rasterize.h"
#include "config.h"

#include <stddef.h>   /* NULL */


/* Canvas size in 1/8-pixel (Q3) units: 28 * 8 = 224. */
#define CANVAS_Q3   (BITMAP_HEIGHT * RAST_SUB)

/* Centerline span: the ink adds one brush radius on each side, so the
 * centerline gets the target size minus two radii. */
#define SPAN_Q3     (RAST_TARGET_PX * RAST_SUB - 2 * RAST_INK)

/* Gray falloff: 128 exactly at the ink radius, about one pixel of soft edge.
 * gray = 128 + 127 * (R^2 - d^2) / EDGE, clamped to 0..255 */
#define R2          (RAST_INK * RAST_INK)
#define EDGE        (2 * RAST_INK * RAST_SUB)
/* Past this distance the gray is 0, so the brush stops looking. */
#define REACH_Q3    (RAST_INK + RAST_SUB + 1)

static uint8_t gray[BITMAP_WIDTH][BITMAP_HEIGHT];

typedef struct {
    uint16_t min_x, min_y, max_x, max_y;
} bbox_t;

/* Apply the touchpad orientation swap. Flips are done after scaling. */
static void raw_xy(const stroke_pt_t *p, uint16_t *x, uint16_t *y)
{
#if RAST_SWAP_XY
    *x = p->y;
    *y = p->x;
#else
    *x = p->x;
    *y = p->y;
#endif
}

/* Bounding box of all pen_down points. Returns the number of them. */
static uint16_t find_bbox(const stroke_pt_t *pts, uint16_t n, bbox_t *bb)
{
    uint16_t count = 0;
    for (uint16_t i = 0; i < n; i++) {
        if (!pts[i].pen_down) {
            continue;
        }
        uint16_t x, y;
        raw_xy(&pts[i], &x, &y);
        if (count == 0) {
            bb->min_x = bb->max_x = x;
            bb->min_y = bb->max_y = y;
        } else {
            if (x < bb->min_x) bb->min_x = x;
            if (x > bb->max_x) bb->max_x = x;
            if (y < bb->min_y) bb->min_y = y;
            if (y > bb->max_y) bb->max_y = y;
        }
        count++;
    }
    return count;
}

/* Scale factor and offsets, shared by every point of one character. */
typedef struct {
    bbox_t   bb;
    uint32_t span;       /* longest raw side; 0 for a single dot */
    int32_t  sw, sh;     /* scaled width and height, Q3 */
    int32_t  off_x;      /* Q3 offset that centers the scaled stroke */
    int32_t  off_y;
} xform_t;

static void make_xform(const bbox_t *bb, xform_t *t)
{
    uint32_t w = (uint32_t)(bb->max_x - bb->min_x);
    uint32_t h = (uint32_t)(bb->max_y - bb->min_y);
    t->bb   = *bb;
    t->span = (w > h) ? w : h;

    /* One scale for both axes keeps the aspect ratio, like EMNIST. */
    t->sw = 0;
    t->sh = 0;
    if (t->span != 0) {
        t->sw = (int32_t)((w * SPAN_Q3 + t->span / 2) / t->span);
        t->sh = (int32_t)((h * SPAN_Q3 + t->span / 2) / t->span);
    }
    t->off_x = (CANVAS_Q3 - t->sw) / 2;
    t->off_y = (CANVAS_Q3 - t->sh) / 2;
}

/* Raw point -> canvas position in 1/8 pixels. */
static void to_q3(const xform_t *t, const stroke_pt_t *p, int32_t *qx, int32_t *qy)
{
    uint16_t x, y;
    raw_xy(p, &x, &y);
    int32_t sx = 0, sy = 0;
    if (t->span != 0) {
        sx = (int32_t)(((uint32_t)(x - t->bb.min_x) * SPAN_Q3 + t->span / 2) / t->span);
        sy = (int32_t)(((uint32_t)(y - t->bb.min_y) * SPAN_Q3 + t->span / 2) / t->span);
    }
#if RAST_FLIP_X
    sx = t->sw - sx;
#endif
#if RAST_FLIP_Y
    sy = t->sh - sy;
#endif
    *qx = t->off_x + sx;
    *qy = t->off_y + sy;
}

/* Stamp the round brush centered at (qx, qy), in 1/8 pixels.
 * Each pixel keeps the darkest gray any stamp has given it. */
static void stamp(int32_t qx, int32_t qy)
{
    int32_t c0 = (qx - REACH_Q3) / RAST_SUB, c1 = (qx + REACH_Q3) / RAST_SUB;
    int32_t r0 = (qy - REACH_Q3) / RAST_SUB, r1 = (qy + REACH_Q3) / RAST_SUB;
    if (c0 < 0) c0 = 0;
    if (r0 < 0) r0 = 0;
    if (c1 > BITMAP_HEIGHT - 1) c1 = BITMAP_HEIGHT - 1;
    if (r1 > BITMAP_WIDTH - 1) r1 = BITMAP_WIDTH - 1;

    for (int32_t r = r0; r <= r1; r++) {
        for (int32_t c = c0; c <= c1; c++) {
            /* Pixel centers sit at the middle of each 8x8 block. */
            int32_t dx = c * RAST_SUB + RAST_SUB / 2 - qx;
            int32_t dy = r * RAST_SUB + RAST_SUB / 2 - qy;
            int32_t g  = 128 + 127 * (R2 - (dx * dx + dy * dy)) / EDGE;
            if (g > 255) g = 255;
            if (g > gray[r][c]) {
                gray[r][c] = (uint8_t)g;
            }
        }
    }
}

/* Bresenham from (x0, y0) to (x1, y1) in 1/8-pixel steps, stamping as it goes.
 * The last point is stamped too, so a one-point stroke still leaves a dot. */
static void draw_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    int32_t dx = (x1 > x0) ? x1 - x0 : x0 - x1;
    int32_t dy = (y1 > y0) ? y0 - y1 : y1 - y0;   /* negative */
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t err = dx + dy;
    for (;;) {
        stamp(x0, y0);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        int32_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/* Draw every stroke. Returns the number of separate strokes. */
static uint16_t draw_strokes(const stroke_pt_t *pts, uint16_t n, const xform_t *t)
{
    uint16_t strokes = 0;
    int have_last = 0;
    int32_t lx = 0, ly = 0;

    for (uint16_t i = 0; i < n; i++) {
        if (!pts[i].pen_down) {
            have_last = 0;            /* stroke break: don't join across it */
            continue;
        }
        int32_t qx, qy;
        to_q3(t, &pts[i], &qx, &qy);
        if (have_last) {
            draw_line(lx, ly, qx, qy);
        } else {
            draw_line(qx, qy, qx, qy);
            strokes++;
        }
        lx = qx;
        ly = qy;
        have_last = 1;
    }
    return strokes;
}

int rasterize(const stroke_pt_t *pts, uint16_t n,
              uint64_t out[BITMAP_WORDS], rast_stats_t *st)
{
    bitmap_clear(out);

    bbox_t bb;
    uint16_t n_down = find_bbox(pts, n, &bb);
    if (n_down == 0) {
        return -1;
    }

    for (int r = 0; r < BITMAP_WIDTH; r++) {
        for (int c = 0; c < BITMAP_HEIGHT; c++) {
            gray[r][c] = 0;
        }
    }

    xform_t t;
    make_xform(&bb, &t);
    uint16_t strokes = draw_strokes(pts, n, &t);

    /* Binary ink box. A gray >= threshold is ink, matching training. */
    int top = BITMAP_WIDTH, bot = -1, left = BITMAP_HEIGHT, right = -1;
    for (int r = 0; r < BITMAP_WIDTH; r++) {
        for (int c = 0; c < BITMAP_HEIGHT; c++) {
            if (gray[r][c] >= RAST_THRESHOLD) {
                if (r < top)   top = r;
                if (r > bot)   bot = r;
                if (c < left)  left = c;
                if (c > right) right = c;
            }
        }
    }
    if (bot < 0) {
        return -1;                    /* brush too small to leave any ink */
    }

    /* EMNIST centering: the box starts at floor((29 - size) / 2). */
    int h  = bot - top + 1;
    int w  = right - left + 1;
    int dr = (29 - h) / 2 - top;
    int dc = (29 - w) / 2 - left;

    uint16_t ink = 0;
    for (int r = top; r <= bot; r++) {
        for (int c = left; c <= right; c++) {
            if (gray[r][c] >= RAST_THRESHOLD) {
                bitmap_set(out, r + dr, c + dc);   /* box <= 28, always in range */
                ink++;
            }
        }
    }

    if (st != NULL) {
        st->n_points  = n_down;
        st->n_strokes = strokes;
        st->raw_w     = (uint16_t)(bb.max_x - bb.min_x);
        st->raw_h     = (uint16_t)(bb.max_y - bb.min_y);
        st->box_rows  = (uint8_t)h;
        st->box_cols  = (uint8_t)w;
        st->ink       = ink;
    }
    return 0;
}