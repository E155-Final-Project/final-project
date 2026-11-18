// vga_pll.sv
// PLL wrapper: 48 MHz HSOSC -> ~25.175 MHz VGA pixel clock.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Clock plan:
//   HSOSC (48 MHz, internal) -> PLL REFERENCECLK -> pix_clk (25.125 MHz)
//   The internal-oscillator-fed PLL keeps every GPIO free (no _PAD variant).
//
// Target: 640x480 @ 60 Hz VESA wants 25.175 MHz; 25.125 MHz is -0.2 %,
//   giving 31.41 kHz line rate and 59.82 Hz frame rate. Monitors accept this.
//
// PLL math (iCE40 sysCLOCK):
//   f_pfd = f_ref / (DIVR+1)            must be 10..133 MHz
//   f_vco = f_pfd * (DIVF+1)            must be 533..1066 MHz
//   f_out = f_vco / 2^DIVQ
//
//   48 MHz ref:  DIVR = 3  -> f_pfd = 12 MHz
//                DIVF = 66 -> f_vco = 804 MHz
//                DIVQ = 5  -> f_out = 25.125 MHz
//                FILTER_RANGE = 1
//   (If HSOSC is ever set to 24 MHz: DIVR = 1, same DIVF/DIVQ.)
//   Verify against `icepll -i 48 -o 25.175` or the Radiant PLL IP wizard.
//
// OPEN DECISION: primitive name depends on the toolchain.
//   - Radiant (iCE40 UltraPlus):  PLL_B
//       ports: REFERENCECLK, OUTCORE, OUTGLOBAL, FEEDBACK, LOCK,
//              RESET_N, BYPASS, ...  (verify against Radiant primitive library)
//   - yosys / iCEcube2:           SB_PLL40_CORE
//       ports: REFERENCECLK, PLLOUTCORE, PLLOUTGLOBAL, LOCK, RESETB, BYPASS
//   This file lives in the Radiant project, so PLL_B is the likely choice.
//
// Outputs:
//   pix_clk : use the GLOBAL output so it rides a global clock net
//   locked  : high once the PLL is stable; vga_top holds reset until then
//
// TODO:
//   - pick primitive (PLL_B vs SB_PLL40_CORE) and instantiate it
//   - set DIVR / DIVF / DIVQ / FILTER_RANGE / FEEDBACK_PATH = "SIMPLE"
//   - tie RESET_N/RESETB high, BYPASS low
//   - (sim) `ifdef SIMULATION: replace the primitive with a behavioral
//     clock so vga_sync_tb doesn't need vendor libraries

module vga_pll (
    input  logic ref_clk,     // 48 MHz from HSOSC
    output logic pix_clk,     // ~25.125 MHz pixel clock
    output logic locked       // PLL lock indicator
);

    // TODO

endmodule