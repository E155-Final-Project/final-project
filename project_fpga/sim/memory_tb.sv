`timescale 1ns/1ps
`include "BNN_params.svh"

// memory testbench.
// 1. Writes l0..l3_weights.mem into the SPRAM through the load port.
// 2. Reads every weight word back (one-clock latency) and compares.
// 3. Reads every threshold for layers 0..2 and checks that layer 3 gives 0.
//
// Run with the working directory set to BNN/mem (BNN_thresh opens
// l<i>_thresh.mem by bare name), or copy the .mem files next to the sim.
// Needs the Radiant iCE40UP simulation library for SP256K.

module memory_tb;

    localparam int L0_BASE = 0;
    localparam int L1_BASE = L0_BASE + L0_Neurons * L0_WORDS;   // 6656
    localparam int L2_BASE = L1_BASE + L1_Neurons * L1_WORDS;   // 10752
    localparam int L3_BASE = L2_BASE + L2_Neurons * L2_WORDS;   // 14848
    localparam int N_WEIGHT_WORDS = L3_BASE + L3_N_OUT * L3_WORDS;  // 15224

    logic        clk = 0;
    logic        load_we;
    logic [13:0] load_addr;
    logic [63:0] load_data;
    logic [13:0] weight_addr;
    logic [1:0]  layer;
    logic [9:0]  neuron;
    logic [63:0] weight;
    logic [9:0]  thresh;

    memory dut (.*);

    always #10 clk = ~clk;

    logic [63:0] w_exp [0:N_WEIGHT_WORDS-1];
    logic [9:0]  t0 [0:511], t1 [0:511], t2 [0:511];

    function automatic logic [9:0] t_exp(int l, int n);
        case (l)
            0:       return t0[n];
            1:       return t1[n];
            2:       return t2[n];
            default: return 10'd0;   // output layer has no threshold
        endcase
    endfunction

    int errors = 0;

    initial begin
        $readmemh("l0_weights.mem", w_exp, L0_BASE, L1_BASE - 1);
        $readmemh("l1_weights.mem", w_exp, L1_BASE, L2_BASE - 1);
        $readmemh("l2_weights.mem", w_exp, L2_BASE, L3_BASE - 1);
        $readmemh("l3_weights.mem", w_exp, L3_BASE, N_WEIGHT_WORDS - 1);
        $readmemh("l0_thresh.mem",  t0);
        $readmemh("l1_thresh.mem",  t1);
        $readmemh("l2_thresh.mem",  t2);

        load_we = 0; load_addr = 0; load_data = 0;
        weight_addr = 0; layer = 0; neuron = 0;
        @(negedge clk);

        // 1. load
        for (int a = 0; a < N_WEIGHT_WORDS; a++) begin
            load_we = 1; load_addr = a; load_data = w_exp[a];
            @(negedge clk);
        end
        load_we = 0;
        @(negedge clk);

        // 2. weight read-back: address set at a negedge, data valid after the next posedge
        for (int a = 0; a < N_WEIGHT_WORDS; a++) begin
            weight_addr = a;
            @(negedge clk);
            if (weight !== w_exp[a]) begin
                if (errors < 10)
                    $display("weight[%0d] = %h, expected %h", a, weight, w_exp[a]);
                errors++;
            end
        end

        // 3. thresholds
        for (int l = 0; l < N_LAYERS; l++) begin
            for (int n = 0; n < 512; n++) begin
                layer = l; neuron = n;
                @(negedge clk);
                if (thresh !== t_exp(l, n)) begin
                    if (errors < 10)
                        $display("thresh[L%0d][%0d] = %0d, expected %0d",
                                 l, n, thresh, t_exp(l, n));
                    errors++;
                end
            end
        end

        if (errors == 0) $display("PASS: %0d weight words, %0d thresholds", N_WEIGHT_WORDS, 3*512);
        else             $display("FAIL: %0d mismatches", errors);
        $finish;
    end

endmodule
