`timescale 1ns/1ps

// BNN testbench: memory + BNN_core, no SPI.
//   1. Load every weight word through memory's load port.
//   2. For each image: hold the core in reset, load the 13 image words,
//      release the core, wait for done, write class_out to +PRED_FILE.
//
// Run from BNN/ with: make sim VEC=smoke   (or VEC=full for all 18,800 images)
// Needs the Radiant iCE40UP library (VSIM_FLAGS="-L iCE40UP") for SP256K and SB_RAM256x16.

module tb_bnn;

    import BNN_params::*;
    localparam bnn_params_t P = BNN_PARAMS;

    `include "vectors.svh"      // N_IMAGES, HAS_TRACE

    localparam int L1_BASE        = P.L0_NEURONS * P.L0_WORDS;
    localparam int L2_BASE        = L1_BASE + P.L1_NEURONS * P.L1_WORDS;
    localparam int L3_BASE        = L2_BASE + P.L2_NEURONS * P.L2_WORDS;
    localparam int N_WEIGHT_WORDS = L3_BASE + P.L3_NEURONS * P.L3_WORDS;
    localparam int IMG_WORDS      = P.L0_WORDS;
    localparam int TIMEOUT        = 100_000;    // clocks; one inference is about 17,000

    // ---- DUT: memory + core, wired like BNN_top ----
    logic        clk = 0, core_reset;
    logic        new_word, load_start, load_weights;
    logic [63:0] load_data;
    logic [63:0] data_in, weight;
    logic  [9:0] thresh;
    logic  [4:0] word;
    logic  [1:0] layer;
    logic  [9:0] neuron;
    logic [13:0] weight_addr;
    logic        act_bit, act_we, done;
    logic  [7:0] class_out;

    always #10 clk = ~clk;

    memory #(.P(P)) mem (
        .clk(clk),
        .new_word(new_word), .load_start(load_start), .load_weights(load_weights), .load_data(load_data),
        .weight_addr(weight_addr), .layer(layer), .word(word), .neuron(neuron),
        .act_bit(act_bit), .act_we(act_we),
        .weight(weight), .thresh(thresh), .activations(data_in));

    BNN_core #(.P(P)) core (
        .clk(clk), .reset(core_reset), .done(done),
        .data_in(data_in), .weight(weight), .thresh(thresh),
        .word(word), .layer(layer), .neuron(neuron),
        .weight_addr(weight_addr), .class_out(class_out),
        .act_bit(act_bit), .act_we(act_we));

    // ---- golden data ----
    logic [63:0] w_exp    [0:N_WEIGHT_WORDS-1];
    logic [63:0] inputs   [0:N_IMAGES*IMG_WORDS-1];
    logic  [7:0] expected [0:N_IMAGES-1];

    // ---- load port helpers (drive on the falling edge) ----
    task automatic begin_load(input bit weights);
        load_weights = weights;
        load_start   = 1;
        @(negedge clk);
        load_start   = 0;
    endtask

    task automatic write_word(input logic [63:0] w);
        new_word  = 1;
        load_data = w;
        @(negedge clk);
        new_word  = 0;
    endtask

    // ---- test ----
    string mem_dir, vec_dir, pred_file;
    int    fd, cycles, mismatches = 0;

    initial begin
        if (!$value$plusargs("MEM_DIR=%s",   mem_dir))   mem_dir   = "../../BNN/mem";
        if (!$value$plusargs("VEC_DIR=%s",   vec_dir))   vec_dir   = "../../BNN/vectors/smoke";
        if (!$value$plusargs("PRED_FILE=%s", pred_file)) pred_file = "predictions.txt";

        $readmemh({mem_dir, "/l0_weights.mem"}, w_exp, 0,       L1_BASE - 1);
        $readmemh({mem_dir, "/l1_weights.mem"}, w_exp, L1_BASE, L2_BASE - 1);
        $readmemh({mem_dir, "/l2_weights.mem"}, w_exp, L2_BASE, L3_BASE - 1);
        $readmemh({mem_dir, "/l3_weights.mem"}, w_exp, L3_BASE, N_WEIGHT_WORDS - 1);
        $readmemh({vec_dir, "/inputs.mem"},     inputs);
        $readmemh({vec_dir, "/expected.mem"},   expected);

        fd = $fopen(pred_file, "w");

        core_reset = 1;
        new_word = 0; load_start = 0; load_weights = 0; load_data = 0;
        @(negedge clk);

        // 1. weights
        begin_load(1);
        for (int a = 0; a < N_WEIGHT_WORDS; a++)
            write_word(w_exp[a]);
        load_weights = 0;

        // 2. images
        for (int i = 0; i < N_IMAGES; i++) begin
            core_reset = 1;
            begin_load(0);
            for (int k = 0; k < IMG_WORDS; k++)
                write_word(inputs[i*IMG_WORDS + k]);
            @(negedge clk);
            core_reset = 0;

            cycles = 0;
            while (!done && cycles < TIMEOUT) begin
                @(negedge clk);
                cycles++;
            end
            if (!done) $error("img %0d: done never rose", i);

            $fdisplay(fd, "%0d", class_out);
            if (class_out !== expected[i]) begin
                if (mismatches < 10)
                    $display("img %0d: class %0d, expected %0d", i, class_out, expected[i]);
                mismatches++;
            end
            if (i % 1000 == 999) $display("%0d / %0d images, %0d mismatches", i + 1, N_IMAGES, mismatches);
        end

        $fclose(fd);
        if (mismatches == 0) $display("PASS: %0d images match expected.mem", N_IMAGES);
        else                 $display("FAIL: %0d of %0d images differ from expected.mem", mismatches, N_IMAGES);
        $finish;
    end

endmodule
