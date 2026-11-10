`include "BNN_params.svh"

// Parameter memory for BNN_core: weights in SPRAM, thresholds in EBR.

module memory (
    input  logic        clk,

    // load port (from the SPI or flash loader)
    input  logic        load_we,
    input  logic [13:0] load_addr,
    input  logic [63:0] load_data,

    // inference port (from BNN_core)
    input  logic [13:0] weight_addr,
    input  logic [1:0]  layer,
    input  logic [9:0]  neuron,
    output logic [63:0] weight,
    output logic [9:0]  thresh
);

    // ---- weights: SPRAM ----
    logic [13:0] spram_addr;
    assign spram_addr = load_we ? load_addr : weight_addr;

    SPRAM_1024KB weight_ram (
        .clk     (clk),
        .wren    (load_we),
        .addr    (spram_addr),
        .mask    (16'hFFFF),        // always write the whole 64-bit word
        .data_in (load_data),
        .data_out(weight)
    );

    // ---- thresholds: EBR ----
    // Hidden layers have 512 neurons, so neuron[9] is always 0 here.
    BNN_thresh thresh_rom (
        .clk   (clk),
        .layer (layer),
        .neuron(neuron[8:0]),
        .thresh(thresh)
    );

endmodule
