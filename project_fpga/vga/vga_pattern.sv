// vga_pattern.sv
// Static bring-up test pattern. No BNN data, no ROMs: pure function of (x, y).
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Layout (640 x 480):
//   +--------------------------------------------------+  <- 1 px white border
//   | blk | blu | red | mag | grn | cyn | yel | wht    |  top half, y < 240:
//   |     |     |     |     |     |     |     |        |  8 vertical bars,
//   |     |     |     |     |     |     |     |        |  80 px each,
//   |-----+-----+-----+-----+-----+-----+-----+--------|  (index = x / 80)
//   | # . # . # . # . # . # . # . # . # . # . # . # .  |  bottom half, y >= 240:
//   | . # . # . # . # . # . # . # . # . # . # . # . #  |  16 px checkerboard,
//   | # . # . # . # . # . # . # . # . # . # . # . # .  |  white / black
//   +--------------------------------------------------+
//
// What each element checks on real hardware:
//   - border:       active region is exactly 640x480, no off-by-one at edges,
//                   monitor isn't cropping (overscan) or shifting the image
//   - color bars:   each color pin/DAC resistor is wired (bar i = {R,G,B} =
//                   i[2], i[1], i[0] at full scale), no swapped channels
//   - checkerboard: pixel clock stable (no shimmer/jitter on square edges),
//                   squares are square (aspect / timing sanity)
//
// Note: 640 / 8 = 80 px per bar, not a power of two, so bar index is a
//   compare chain (or x / 80 as a small LUT), not a bit slice.
//   Checkerboard: checker = x[4] ^ y[4].
//
// Color format: 3R + 3G + 2B. Full scale = r 3'b111, g 3'b111, b 2'b11.
//   Optional extra: a grayscale ramp strip to check DAC linearity (later).
//
// Outputs must be forced to 0 when ~active (blanking) -- monitors use the
//   blanking level as their black reference.
//
// TODO:
//   - border condition (x == 0 | x == 639 | y == 0 | y == 479)
//   - bar index from x, map to {R,G,B} full-scale
//   - checkerboard from x[4] ^ y[4]
//   - mux by region, blank when ~active

module vga_pattern (
    input  logic       active,
    input  logic [9:0] x,
    input  logic [9:0] y,
    output logic [2:0] r,
    output logic [2:0] g,
    output logic [1:0] b
);

    // TODO

endmodule