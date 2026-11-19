// char_shift.sv
// FPGA_B: 8-character rolling log shown on the 2 x 4-digit 14-seg bank.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Behavior (FPGA_B owns the position, FPGA_A just sends characters):
//   - Terminal style: the newest character enters at the right (digit 7),
//     everything else shifts one place left, digit 0's character drops off.
//   - clear: all 8 entries become ASCII space (8'h20) = blank digits.
//   - Reset: same as clear, so the bank powers up blank.
//   - Read port: scan_fsm asks for one digit at a time (combinational).
//
// Storage: 8 x 8-bit registers (64 FFs). Too small to need EBR.

module char_shift (
    input  logic       clk,
    input  logic       rst_n,
    input  logic       append,       // 1-cycle pulse: add char_in at the right
    input  logic       clear,        // 1-cycle pulse: blank the whole log
    input  logic [7:0] char_in,      // ASCII
    input  logic [2:0] rd_digit,     // from scan_fsm: which digit is lit
    output logic [7:0] rd_char       // ASCII for that digit
);

    logic [7:0] chars [7:0];  // 8 x 8-bit registers

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            for (int i = 0; i < 8; i++)
                chars[i] <= 8'h20;  // blank on reset
        end
        else if (clear) begin
            for (int i = 0; i < 8; i++)
                chars[i] <= 8'h20; // blank on clear
        end
        else if (append) begin
            for (int i = 0; i < 7; i++)
                chars[i] <= chars[i+1]; // slide toward index 0
            chars[7] <= char_in;        // newest at the right
        end
    end 

    assign rd_char = chars[rd_digit];

endmodule