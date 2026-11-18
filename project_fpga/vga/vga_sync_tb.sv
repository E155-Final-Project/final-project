// vga_sync_tb.sv
// Self-checking testbench for vga_sync: measures sync periods, pulse widths
// and active-region size in pixel clocks and compares against VESA 640x480@60.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Runs on: ModelSim/Questa (vlog -sv) or Icarus (iverilog -g2012).
//   Drives clk directly, no PLL, so it needs no vendor libraries.
//   Period 39.8 ns (~25.125 MHz) is cosmetic; every check is in clock counts.
//
// Checks (expected value in pixel clocks):
//   hsync period        800        falling edge to falling edge
//   hsync low width      96
//   active width        640        active-high run within one line
//   hsync falls at      hcount 656 (active start + 640 + 16 FP)
//   vsync period    420,000        525 lines * 800
//   vsync low width   1,600        2 lines * 800
//   active lines        480        lines containing any active pixel per frame
//   vblank / frame_start sanity (frame_start once per 420,000 cycles)
//
// Method:
//   - free-running cycle counter; timestamp each hsync_n / vsync_n edge and
//     each rising/falling edge of active
//   - after reset, skip the first (partial) frame, then measure 2 full frames
//   - $error on any mismatch, print a PASS/FAIL summary, $finish
//   - optional: $dumpfile/$dumpvars for GTKWave / wave window
//
// Sim length: ~3 frames = ~1.3 M cycles, a few seconds in Icarus.
//
// TODO:
//   - clock + reset generation
//   - vga_sync DUT instance
//   - edge-timestamp logic and counters
//   - assertions + summary

`timescale 1ns/1ps

module vga_sync_tb;

    localparam int H_TOTAL = 800, H_ACTIVE = 640, H_SYNC = 96;
    localparam int V_TOTAL = 525, V_ACTIVE = 480, V_SYNC = 2;

    logic       clk, reset;
    logic       hsync_n, vsync_n, active, vblank, frame_start;
    logic [9:0] x, y;

    // TODO

endmodule