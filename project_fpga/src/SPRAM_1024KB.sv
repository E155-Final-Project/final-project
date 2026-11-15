// All four iCE40UP5K SPRAM blocks side by side: 16,384 x 64 bit (1 Mbit).
// Uses the Radiant primitive SP256K (iCEcube2/yosys call it SB_SPRAM256KA).
// Reads take one clock. SPRAM contents are not set by the bitstream.
module SPRAM_1024KB (
    input  logic        clk,
    input  logic        wren,      // 1 = write, 0 = read
    input  logic [13:0] addr,      // word address, shared by all 4 blocks
    input  logic [15:0] mask,      // 1 bit per nibble, 1 = write that nibble
    input  logic [63:0] data_in,
    output logic [63:0] data_out
);

    SP256K spram0 (
        .AD      (addr),
        .DI      (data_in[15:0]),
        .MASKWE  (mask[3:0]),
        .WE      (wren),
        .CS      (1'b1),
        .CK      (clk),
        .STDBY   (1'b0),
        .SLEEP   (1'b0),
        .PWROFF_N(1'b1),   // active low: 1 = powered on
        .DO      (data_out[15:0])
    );

    SP256K spram1 (
        .AD      (addr),
        .DI      (data_in[31:16]),
        .MASKWE  (mask[7:4]),
        .WE      (wren),
        .CS      (1'b1),
        .CK      (clk),
        .STDBY   (1'b0),
        .SLEEP   (1'b0),
        .PWROFF_N(1'b1),   // active low: 1 = powered on
        .DO      (data_out[31:16])
    );

    SP256K spram2 (
        .AD      (addr),
        .DI      (data_in[47:32]),
        .MASKWE  (mask[11:8]),
        .WE      (wren),
        .CS      (1'b1),
        .CK      (clk),
        .STDBY   (1'b0),
        .SLEEP   (1'b0),
        .PWROFF_N(1'b1),   // active low: 1 = powered on
        .DO      (data_out[47:32])
    );

    SP256K spram3 (
        .AD      (addr),
        .DI      (data_in[63:48]),
        .MASKWE  (mask[15:12]),
        .WE      (wren),
        .CS      (1'b1),
        .CK      (clk),
        .STDBY   (1'b0),
        .SLEEP   (1'b0),
        .PWROFF_N(1'b1),   // active low: 1 = powered on
        .DO      (data_out[63:48])
    );

endmodule
