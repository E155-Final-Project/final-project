// seg_decoder.sv
// FPGA_B: ASCII character -> 14-segment + decimal point pattern.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Output: 15 bits, 1 = segment lit (logical; polarity is handled in scan_fsm).
//   Bit order: TODO, match the 14-seg display datasheet
//   (datasheets/14-seg-display_ds.pdf), e.g. [0]=A ... [13]=N, [14]=DP.
//
// Characters needed (PROPOSED: FPGA_A sends ASCII, not class numbers):
//   - the 47 EMNIST balanced classes: 0-9, A-Z, a b d e f g h n q r t
//   - space (8'h20) = all off
//   - anything else = a visible "unknown" pattern for debugging (centered star)
//
// Combinational (case statement). 48 entries is small enough for logic;
//
// TODO:
//   - pin down the segment bit order from the datasheet
//   - case table for all 47 characters + space
//   - default branch

module seg_rom (
    input  logic [7:0]  ascii,
    output logic [14:0] segs         // 14 segments + DP, 1 = on
);

    // TODO

endmodule