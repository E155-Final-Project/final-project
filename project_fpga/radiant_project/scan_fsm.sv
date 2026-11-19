// scan_fsm.sv
// FPGA_B: 16-phase scan driver for 2 x 4-digit 14-segment duplex displays
// sharing one 7-bit segment bus.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Phase layout (phase[3:0]):
//   phase[3]   = module select  (0 = module A, 1 = module B)
//   phase[2:1] = digit in module (0..3)
//   phase[0]   = bank            (0 = upper common, 1 = lower common)
//
// Scan math:
//   - 200 Hz per common x 16 phases = 3.2 kHz phase rate
//   - 312.5 us per phase = 15,000 cycles at 48 MHz
//   - BLANK_US of each phase: all commons off while the bus settles, so the
//     old segment pattern can't ghost onto the next common
//
// Polarity: active-high everywhere (NPN sinks on segments, PNP + NPN
// pre-driver pairs on commons -> FPGA HIGH turns the driver ON).

module scan_fsm #(
    parameter int CLK_HZ            = 48_000_000,
    parameter int DIGIT_HZ          = 200,      // refresh per digit
    parameter int BLANK_US          = 5        // gap between digits
) (
    input  logic        clk,
    input  logic        rst_n,
    input  logic [14:0] seg_pattern, // from seg_rom, for the current digit
    output logic [2:0]  digit_idx,   // to char_buffer.rd_digit
    output logic [6:0]  seg_out,     // to the segment drivers (pins)
    output logic [7:0]  digit_a,     // module A commons, one-hot
    output logic [7:0]  digit_b      // module B commons, one-hot
);
    localparam int PHASES = 16;
    localparam int PHASE_CYCLES = CLK_HZ / (DIGIT_HZ * PHASES);
    localparam int BLANK_CYCLES = (CLK_HZ / 1_000_000) * BLANK_US;
    localparam int CNT_W = $clog2(PHASE_CYCLES);
    
    

    logic [CNT_W-1:0]   cycle;
    logic [3:0]         phase;
    logic               active;
    logic bank;
    logic [7:0] one_hot;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            cycle <= '0;
            phase <= '0;
        end 
        else if (cycle == PHASE_CYCLES-1) begin
            cycle <= '0;
            phase <= phase + 1'b1;
        end
        else begin
            cycle <= cycle + 1'b1;
        end
    end

    assign active       = (cycle >= BLANK_CYCLES);
    assign digit_idx    = {phase[3], phase[2:1]};
    assign bank         = phase[0];

    assign seg_out = bank ? seg_pattern[13:7]   // lower: {N,M,L,K,J,H,G2}
                          : seg_pattern[6:0];   // upper: {G1,F,E,D,C,B,A}

    assign one_hot = active ? (8'b1 << phase[2:0]) : 8'b0;
    assign digit_a = (phase[3] == 1'b0) ? one_hot : 8'b0;
    assign digit_b = (phase[3] == 1'b1) ? one_hot : 8'b0;

endmodule