// vga_sync.sv
// 640x480 @ 60 Hz VESA timing generator: H/V counters, sync pulses,
// active-video flag, pixel coordinates.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Timing (all in pixel clocks / lines):
//   H: 640 active | 16 front porch | 96 sync | 48 back porch = 800 total
//   V: 480 active | 10 front porch |  2 sync | 33 back porch = 525 total
//   Both syncs are NEGATIVE polarity (low during the sync pulse).
//
// Counter convention:
//   hcount runs 0..799, vcount runs 0..524. Active region is
//   hcount < 640 && vcount < 480, so x = hcount and y = vcount directly.
//
//   hsync low when  656 <= hcount < 752   (640 + 16 .. 640 + 16 + 96 - 1)
//   vsync low when  490 <= vcount < 492   (480 + 10 .. 480 + 10 +  2 - 1)
//
// Outputs are registered so hsync/vsync/active/x/y all line up on the same
// clock edge. Downstream pixel logic that adds pipeline stages (font ROM
// read, etc.) must delay hsync/vsync/active by the same number of cycles.
//
// vblank / frame_start are for later: vga_top uses vblank to time the
// top10_buffer snapshot (CDC), frame_start is a handy one-cycle strobe.
//
// Verified by: project sim/vga_sync_tb.sv (hsync period 800, hsync low 96,
//   active width 640, vsync period 420,000, vsync low 1,600, 480 active lines)
//
// TODO:
//   - hcount: increment, wrap at H_TOTAL-1
//   - vcount: increment when hcount wraps, wrap at V_TOTAL-1
//   - registered hsync_n, vsync_n, active, x, y
//   - vblank = (vcount >= V_ACTIVE); frame_start = (hcount == 0 && vcount == 0)
//   - synchronous reset to (0, 0)

module vga_sync #(
    parameter int H_ACTIVE = 640,
    parameter int H_FP     = 16,
    parameter int H_SYNC   = 96,
    parameter int H_BP     = 48,
    parameter int V_ACTIVE = 480,
    parameter int V_FP     = 10,
    parameter int V_SYNC   = 2,
    parameter int V_BP     = 33
) (
    input  logic       clk,          // pixel clock
    input  logic       reset,        // synchronous, active high
    output logic       hsync_n,      // active low
    output logic       vsync_n,      // active low
    output logic       active,       // 1 inside the 640x480 visible region
    output logic       vblank,       // 1 during vertical blanking (lines 480..524)
    output logic       frame_start,  // 1-cycle pulse at (0, 0)
    output logic [9:0] x,            // 0..799 (valid pixel when active)
    output logic [9:0] y             // 0..524 (valid line when active)
);

    localparam int H_TOTAL = H_ACTIVE + H_FP + H_SYNC + H_BP;   // 800
    localparam int V_TOTAL = V_ACTIVE + V_FP + V_SYNC + V_BP;   // 525

    // TODO

endmodule