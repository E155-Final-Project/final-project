`timescale 1ns/1ps

// Activation_EBR testbench, driven by the golden smoke vectors (16 images).
// For each image:
//   1. load the 13 input words into the top half through the load port
//   2. for layers 0..2: shift in the golden activation bits, one neuron per
//      cycle, while reading this layer's input half (checks that a read and
//      a write to the two halves can happen on the same cycle)
//   3. read the finished output half back as the next layer, and compare
//      with l<L>_act.mem
// At the end, the bottom half must hold layer 2's output, the output layer's input.
//
// Run with the working directory set to BNN/vectors/smoke.

module activation_tb;

    localparam int N_IMAGES = 16;
    localparam int IN_WORDS = 13;
    localparam int H_WORDS  = 8;
    localparam int H_NEURONS = 512;

    logic        clk = 0;
    logic        load_we;
    logic [3:0]  load_addr;
    logic [63:0] load_data;
    logic [1:0]  layer;
    logic [4:0]  word;
    logic [9:0]  neuron;
    logic        act_bit, act_we;
    logic [63:0] activations;

    Activation_EBR dut (.*);

    always #10 clk = ~clk;

    logic [63:0] inputs [0:N_IMAGES*IN_WORDS-1];
    logic [63:0] act0   [0:N_IMAGES*H_WORDS-1];
    logic [63:0] act1   [0:N_IMAGES*H_WORDS-1];
    logic [63:0] act2   [0:N_IMAGES*H_WORDS-1];

    int errors = 0;

    // golden word k of layer L's INPUT for image i (layer 0 = image, layer L = act L-1)
    function automatic logic [63:0] src_word(int L, int i, int k);
        case (L)
            0:       return inputs[i*IN_WORDS + k];
            1:       return act0[i*H_WORDS + k];
            2:       return act1[i*H_WORDS + k];
            default: return act2[i*H_WORDS + k];
        endcase
    endfunction

    task automatic check(string what, logic [63:0] got, logic [63:0] exp);
        if (got !== exp) begin
            if (errors < 10) $display("%s: got %h, expected %h", what, got, exp);
            errors++;
        end
    endtask

    // read words 0..n-1 of layer L's input buffer and compare
    task automatic read_back(int L, int i, int n);
        for (int k = 0; k < n; k++) begin
            layer = L; word = k;
            @(negedge clk);
            check($sformatf("img %0d L%0d in word %0d", i, L, k), activations, src_word(L, i, k));
        end
    endtask

    initial begin
        $readmemh("inputs.mem", inputs);
        $readmemh("l0_act.mem", act0);
        $readmemh("l1_act.mem", act1);
        $readmemh("l2_act.mem", act2);

        load_we = 0; load_addr = 0; load_data = 0;
        layer = 0; word = 0; neuron = 0; act_bit = 0; act_we = 0;
        @(negedge clk);

        for (int i = 0; i < N_IMAGES; i++) begin
            // 1. load the image
            for (int k = 0; k < IN_WORDS; k++) begin
                load_we = 1; load_addr = k; load_data = inputs[i*IN_WORDS + k];
                @(negedge clk);
            end
            load_we = 0;
            read_back(0, i, IN_WORDS);

            // 2. compute layers 0..2 (activations come from the golden vectors)
            for (int L = 0; L < 3; L++) begin
                int n_in, k;
                logic [63:0] out_word;
                n_in = (L == 0) ? IN_WORDS : H_WORDS;
                for (int n = 0; n < H_NEURONS; n++) begin
                    out_word = src_word(L + 1, i, n / 64);
                    k        = n % n_in;
                    layer    = L;
                    neuron   = n;
                    act_bit  = out_word[n % 64];
                    act_we   = 1;
                    word     = k;
                    @(negedge clk);
                    check($sformatf("img %0d L%0d in word %0d during write", i, L, k),
                          activations, src_word(L, i, k));
                end
                act_we = 0;

                // 3. read back as the next layer's input
                read_back(L + 1, i, H_WORDS);
            end
        end

        if (errors == 0) $display("PASS: %0d images, layers 0..2", N_IMAGES);
        else             $display("FAIL: %0d mismatches", errors);
        $finish;
    end

endmodule
