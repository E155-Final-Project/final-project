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

    genvar i;
    generate
        for (i = 0; i < 4; i++) begin : blk
            SP256K spram (
                .AD      (addr),
                .DI      (data_in [16*i +: 16]),
                .MASKWE  (mask    [4*i  +: 4]),
                .WE      (wren),
                .CS      (1'b1),
                .CK      (clk),
                .STDBY   (1'b0),
                .SLEEP   (1'b0),
                .PWROFF_N(1'b1),   // active low: 1 = powered on
                .DO      (data_out[16*i +: 16])
            );
        end
    endgenerate

endmodule
