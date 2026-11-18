// vga_top.sv
// Standalone VGA bring-up top for FPGA_A: HSOSC -> PLL -> sync -> pattern -> pins.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Data path:
//   HSOSC (48 MHz) -> vga_pll -> pix_clk
//                                  |
//                     reset sync --+--> vga_sync -> (x, y, active) -> vga_pattern
//                                          |                              |
//                                    hsync_n, vsync_n              r, g, b
//                                          |                              |
//                                          +------> output regs <---------+
//                                                        |
//                                                   10 VGA pins
//
// Pins: 3R + 3G + 2B + HSYNC + VSYNC = 10. Assignments go in the .ldc.
//   TODO: assign once the VGA breakout / resistor DAC is wired.
//
// Bring-up only: this module owns its own HSOSC because BNN_top is read-only.
//   At integration, vga_top loses its HSOSC (takes ref_clk as a port) and
//   becomes a sibling of BNN_core under FPGA_A's top level, plus a
//   top10_buffer snapshot input (toggle-flag CDC, copied during vblank).
//
// Reset: hold the pixel domain in reset until the PLL reports lock, then
//   release through a 2-flop synchronizer on pix_clk.
//
// Output registers: register r/g/b/hsync_n/vsync_n once more right before
//   the pins so all 10 outputs change on the same edge (and can be packed
//   into IO flops). Keep sync and color in the same pipeline stage.
//
// TODO:
//   - HSOSC #(.CLKHF_DIV(2'b00)) instance (48 MHz), match BNN_top.sv
//   - vga_pll instance
//   - lock-gated reset synchronizer on pix_clk
//   - vga_sync, vga_pattern instances
//   - output register stage -> pins

module vga_top (
    output logic [2:0] vga_r,
    output logic [2:0] vga_g,
    output logic [1:0] vga_b,
    output logic       vga_hsync_n,
    output logic       vga_vsync_n
);

    // TODO

endmodule