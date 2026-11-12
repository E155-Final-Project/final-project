// char_shift.sv
// FPGA_B: 16-character rolling log shown on the 14-segment bank.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Behavior (PROPOSED: FPGA_B owns the position, FPGA_A just sends characters):
//   - Terminal style: the newest character enters at the right (digit 15),
//     everything else shifts one place left, digit 0's character drops off.
//   - clear: all 16 entries become ASCII space (8'h20) = blank digits.
//   - Reset: same as clear, so the bank powers up blank.
//   - Read port: scan_fsm asks for one digit at a time.
//
// Storage: 16 x 8-bit registers (128 FFs). Too small to need EBR.
//
// TODO:
//   - register array logic [7:0] chars [16]
//   - on append: shift left, chars[15] <= char_in
//   - on clear or reset: fill with 8'h20
//   - rd_char = chars[rd_digit] (combinational read)

module char_shift (
    input  logic       clk,
    input  logic       rst_n,
    input  logic       append,       // 1-cycle pulse: add char_in at the right
    input  logic       clear,        // 1-cycle pulse: blank the whole log
    input  logic [7:0] char_in,      // ASCII
    input  logic [3:0] rd_digit,     // from scan_fsm: which digit is lit
    output logic [7:0] rd_char       // ASCII for that digit
);

    // TODO

endmodule