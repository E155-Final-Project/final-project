// fpga_b_top.sv
// FPGA_B top level: SPI link in, 2 x 4-digit 14-seg duplex displays out.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Data path:
//   spi_slave -> cmd decode -> char_shift -> seg_rom -> scan_fsm -> pins
//                                  ^                       |
//                                  +------ rd_digit -------+
//
// Pins (26 of 39 on iCE40 UP5K SG48):
//   3 SPI in (sck, mosi, cs_n) + 7 shared segment bus
//   + 16 digit commons (8 per module, one-hot active per scan phase).
//   Pin assignments go in the .pdc.
//
// Clock: SB_HFOSC at 48 MHz (no PLL needed on FPGA_B).
// Reset: 15-cycle power-on reset counter.
//
// DEMO_MODE = 1: ignore SPI; append the next character of a test string every
// 0.5 s. Proves the driver board, segment map and brightness before FPGA_A
// exists.

module fpga_b_top #(
    parameter bit DEMO_MODE = 0
) (
    input  logic       sck,         // from FPGA_A
    input  logic       mosi,
    input  logic       cs_n,
    output logic [6:0] seg,         // shared 7-bit segment bus (both modules)
    output logic [7:0] dig_a,       // module A commons, one-hot
    output logic [7:0] dig_b        // module B commons, one-hot
);
    // ---------------- Clock (SB_HFOSC, 48 MHz) ----------------
    logic clk;
    HSOSC hf_osc (.CLKHFPU(1'b1), .CLKHFEN(1'b1), .CLKHF(clk));

    // ---------------- Power-on reset ----------------
    logic [3:0] por_cnt = 4'd0;
    logic       rst_n;
    always_ff @(posedge clk) begin
        if (por_cnt != 4'hF) por_cnt <= por_cnt + 1'b1;
    end
    assign rst_n = (por_cnt == 4'hF);

    // ---------------- SPI slave ----------------
    logic [7:0] spi_cmd, spi_data;
    logic       frame_valid;

    spi_slave u_spi (
        .clk         (clk),
        .rst_n       (rst_n),
        .sck         (sck),
        .mosi        (mosi),
        .cs_n        (cs_n),
        .cmd         (spi_cmd),
        .data        (spi_data),
        .frame_valid (frame_valid)
    );

    // ---------------- Command decode / demo mux (debugging) ----------------
    logic       append, clear;
    logic [7:0] char_in;

    generate if (DEMO_MODE) begin : demo
        // Append a test-string character every ~0.5 s
        localparam int DEMO_LEN    = 9;                       // chars in DEMO_STR
        localparam int TICK_CYCLES = 24_000_000;              // 0.5 s at 48 MHz
        // Packed logic vector, MSB-first: DEMO_STR[71:64] = 'H', [7:0] = ' '
        localparam logic [DEMO_LEN*8-1:0] DEMO_STR = "Hi World ";
        logic [24:0] tick_cnt;
        logic [3:0]  idx;
        logic        tick;

        always_ff @(posedge clk or negedge rst_n) begin
            if (!rst_n) begin
                tick_cnt <= '0;
                idx      <= '0;
            end else if (tick_cnt == TICK_CYCLES-1) begin
                tick_cnt <= '0;
                idx      <= (idx == DEMO_LEN-1) ? 4'd0 : idx + 1'b1;
            end else begin
                tick_cnt <= tick_cnt + 1'b1;
            end
        end
        assign tick    = (tick_cnt == TICK_CYCLES-1);
        assign append  = tick;
        assign clear   = 1'b0;
        // idx 0 -> 'H' (top byte), idx DEMO_LEN-1 -> ' ' (bottom byte)
        assign char_in = DEMO_STR[(DEMO_LEN-1-idx)*8 +: 8];
    end else begin : live
        assign append  = frame_valid && (spi_cmd == 8'h01);
        assign clear   = frame_valid && (spi_cmd == 8'h02);
        assign char_in = spi_data;
    end endgenerate

     // ---------------- Char buffer + glyph ROM ----------------
    logic [2:0] rd_digit;
    logic [7:0] rd_char;
    logic [14:0] seg_pattern;

    char_shift u_buf (
        .clk      (clk),
        .rst_n    (rst_n),
        .append   (append),
        .clear    (clear),
        .char_in  (char_in),
        .rd_digit (rd_digit),
        .rd_char  (rd_char));

    seg_rom u_rom (
        .ascii (rd_char),
        .segs  (seg_pattern)
    );

    // ---------------- Scan FSM ----------------
    scan_fsm u_scan (
        .clk         (clk),
        .rst_n       (rst_n),
        .seg_pattern (seg_pattern),
        .digit_idx   (rd_digit),
        .seg_out     (seg),
        .digit_a     (dig_a),
        .digit_b     (dig_b)
    );



endmodule