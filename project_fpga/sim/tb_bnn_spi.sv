`timescale 1ns/1ps

// Full-system testbench: drives BNN_top's pins the way the MCU will.
//   1. Weights: raise load_weights and load, send 121,792 bytes, drop both.
//   2. For each image: raise load, send 104 bytes, drop load,
//      wait for done, clock out 8 bits and read the class.
// SPI mode 0, 5 MHz, LSB first. Predictions go to +PRED_FILE.
//
// Run from BNN/ with: make sim TB=tb_bnn_spi VEC=smoke VSIM_FLAGS="-L iCE40UP"
// The library is needed for HSOSC, SP256K and SB_RAM256x16.
// Sending the weights is about 195 ms of simulated time, so +N=<images>
// limits how many images run (default: all in the vector set).

module tb_bnn_spi;

    import BNN_params::*;
    localparam bnn_params_t P = BNN_PARAMS;

    `include "vectors.svh"      // N_IMAGES, HAS_TRACE

    localparam int L1_BASE        = P.L0_NEURONS * P.L0_WORDS;
    localparam int L2_BASE        = L1_BASE + P.L1_NEURONS * P.L1_WORDS;
    localparam int L3_BASE        = L2_BASE + P.L2_NEURONS * P.L2_WORDS;
    localparam int N_WEIGHT_WORDS = L3_BASE + P.L3_NEURONS * P.L3_WORDS;
    localparam int IMG_WORDS      = P.L0_WORDS;
    localparam int IMG_BYTES      = IMG_WORDS * 8;   // 104

    localparam real SCK_HALF = 100.0;                // 5 MHz
    localparam real TIMEOUT  = 5_000_000.0;          // 5 ms, one inference is about 0.35 ms

    // ---- DUT ----
    logic sck = 0, sdi = 0, load = 0, load_weights = 0;
    logic sdo, done;

    BNN_top dut (.sck(sck), .sdi(sdi), .sdo(sdo), .load(load),
                 .load_weights(load_weights), .done(done));

    // ---- golden data ----
    logic [63:0] w_exp    [0:N_WEIGHT_WORDS-1];
    logic [63:0] inputs   [0:N_IMAGES*IMG_WORDS-1];
    logic  [7:0] expected [0:N_IMAGES-1];

    // ---- SPI, like the MCU ----
    task automatic send_byte(input logic [7:0] b);
        for (int i = 0; i < 8; i++) begin
            sdi = b[i];                  // LSB first
            #(SCK_HALF) sck = 1;
            #(SCK_HALF) sck = 0;
        end
    endtask

    task automatic read_byte(output logic [7:0] b);
        for (int i = 0; i < 8; i++) begin
            #(SCK_HALF) sck = 1;
            b[i] = sdo;                  // sampled on the rising edge
            #(SCK_HALF) sck = 0;
        end
    endtask

    // ---- test ----
    string     mem_dir, vec_dir, pred_file;
    int        fd, n_run, mismatches = 0;
    logic [7:0] cls;
    realtime   t0;

    initial begin
        if (!$value$plusargs("MEM_DIR=%s",   mem_dir))   mem_dir   = "../../BNN/mem";
        if (!$value$plusargs("VEC_DIR=%s",   vec_dir))   vec_dir   = "../../BNN/vectors/smoke";
        if (!$value$plusargs("PRED_FILE=%s", pred_file)) pred_file = "predictions.txt";
        if (!$value$plusargs("N=%d",         n_run) || n_run > N_IMAGES) n_run = N_IMAGES;

        $readmemh({mem_dir, "/l0_weights.mem"}, w_exp, 0,       L1_BASE - 1);
        $readmemh({mem_dir, "/l1_weights.mem"}, w_exp, L1_BASE, L2_BASE - 1);
        $readmemh({mem_dir, "/l2_weights.mem"}, w_exp, L2_BASE, L3_BASE - 1);
        $readmemh({mem_dir, "/l3_weights.mem"}, w_exp, L3_BASE, N_WEIGHT_WORDS - 1);
        // threshold ROMs: load them from MEM_DIR, whatever the working directory is
        $readmemh({mem_dir, "/l0_thresh.mem"}, dut.memory.thresh_rom.l0_thresh_rom.mem);
        $readmemh({mem_dir, "/l1_thresh.mem"}, dut.memory.thresh_rom.l1_thresh_rom.mem);
        $readmemh({mem_dir, "/l2_thresh.mem"}, dut.memory.thresh_rom.l2_thresh_rom.mem);
        $readmemh({vec_dir, "/inputs.mem"},     inputs);
        $readmemh({vec_dir, "/expected.mem"},   expected);

        fd = $fopen(pred_file, "w");
        #1000;

        // 1. weights
        $display("%t  sending %0d weight words", $time, N_WEIGHT_WORDS);
        load_weights = 1;
        #1000 load = 1;
        #1000;
        for (int k = 0; k < N_WEIGHT_WORDS; k++)
            for (int b = 0; b < 8; b++)
                send_byte(w_exp[k][8*b +: 8]);
        #1000 load = 0;
        #1000 load_weights = 0;
        #1000;

        // 2. images
        $display("%t  classifying %0d images", $time, n_run);
        for (int i = 0; i < n_run; i++) begin
            load = 1;
            #1000;
            for (int j = 0; j < IMG_BYTES; j++)
                send_byte(inputs[i*IMG_WORDS + j/8][8*(j%8) +: 8]);
            #1000 load = 0;
            if (done) $error("img %0d: done high right after load fell", i);

            t0 = $realtime;
            while (!done && $realtime - t0 < TIMEOUT) #100;
            if (!done) $error("img %0d: done never rose", i);

            read_byte(cls);
            $fdisplay(fd, "%0d", cls);
            if (cls !== expected[i]) begin
                if (mismatches < 10)
                    $display("img %0d: class %0d, expected %0d", i, cls, expected[i]);
                mismatches++;
            end
            #1000;
        end

        $fclose(fd);
        if (mismatches == 0) $display("PASS: %0d images over SPI match expected.mem", n_run);
        else                 $display("FAIL: %0d of %0d images differ from expected.mem", mismatches, n_run);
        $finish;
    end

endmodule
