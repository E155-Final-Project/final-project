`include "BNN_params.svh"

// All memory for BNN_core: weights in SPRAM, thresholds in EBR ROM,
// and the ping-pong activation buffer in EBR.

module memory (
    input  logic        clk,

    // load port (from the SPI, already synchronized to clk)
    input  logic        new_word,      // one pulse per 64-bit word in load_data
    input  logic        load_start,    // one pulse when load rises: address back to 0
    input  logic        load_weights,  // 1: words go to the weights, 0: to the image
    input  logic [63:0] load_data,

    // inference port (from BNN_core)
    input  logic [13:0] weight_addr,
    input  logic [1:0]  layer,
    input  logic [4:0]  word,
    input  logic [9:0]  neuron,
    input  logic        act_bit,
    input  logic        act_we,
    output logic [63:0] weight,
    output logic [9:0]  thresh,
    output logic [63:0] activations
);

    // ---- load: steer each word and count addresses ----
    // the address resets when load rises, not while load is low,
    // so the last word still gets its address
    logic [13:0] load_addr;
    always_ff @(posedge clk)
        if (load_start)    load_addr <= 0;
        else if (new_word) load_addr <= load_addr + 1;

    logic w_we, img_we;
    assign w_we   = new_word &  load_weights;
    assign img_we = new_word & ~load_weights;

    // ---- weights: SPRAM ----
    logic [13:0] spram_addr;
    assign spram_addr = w_we ? load_addr : weight_addr;

    SPRAM_1024KB weight_ram (
        .clk     (clk),
        .wren    (w_we),
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

    // ---- activations: EBR ----
    Activation_EBR act_ram (
        .clk        (clk),
        .load_we    (img_we),
        .load_addr  (load_addr[3:0]),
        .load_data  (load_data),
        .layer      (layer),
        .word       (word),
        .neuron     (neuron),
        .act_bit    (act_bit),
        .act_we     (act_we),
        .activations(activations)
    );

endmodule
