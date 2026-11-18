// scan_fsm.sv
// FPGA_B: time-multiplexes the 16 x 14-segment displays.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Scan math (from the output-hardware post):
//   - 200 Hz per digit x 16 digits = 3.2 kHz digit rate
//   - 312.5 us per digit = 15,000 cycles at 48 MHz
//   - blanking gap at each switch: all digits off for BLANK_US, so the old
//     segment pattern doesn't ghost onto the next digit
//
// Sequence per digit slot:
//   1. all digit enables off (blank gap), advance digit index
//   2. segment bus takes the new digit's pattern (seg_rom output)
//   3. after the gap, enable only the current digit for the rest of the slot
//
// Polarity: depends on the driver board, so it's a parameter.
//   - UDN2981 (digits) + ULN2803 (segments): both active high
//   - discrete PNP digit drivers: digits active LOW
//   Verify against the driver and display datasheets before wiring.
//
// TODO:
//   - cycle counter for the slot length, derived from the parameters
//   - 4-bit digit index (wraps 15 -> 0)
//   - blank-gap logic
//   - output polarity handling

module scan_fsm #(
    parameter int CLK_HZ            = 48_000_000,
    parameter int DIGIT_HZ          = 200,      // refresh per digit
    parameter int BLANK_US          = 5,        // gap between digits
    parameter bit DIGIT_ACTIVE_HIGH = 1,        // 0 for discrete PNP drivers
    parameter bit SEG_ACTIVE_HIGH   = 1
) (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [14:0] seg_pattern, // from seg_rom, for the current digit
    output logic [3:0]  digit_idx,   // to char_buffer.rd_digit
    output logic [14:0] seg_out,     // to the segment drivers (pins)
    output logic [15:0] digit_en     // to the digit drivers (pins), one-hot
);

    // TODO

endmodule