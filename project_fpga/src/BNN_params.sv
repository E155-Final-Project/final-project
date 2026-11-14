// BNN layer sizes for fpga_balanced_h512-512-512.weights.h5 (784 -> 512 -> 512 -> 512 -> 47).
// Must match BNN/mem/bnn_params.svh, which gen_vectors.py writes.
// Compile this file before the modules that use it.

package BNN_params;

    typedef struct packed {
        int N_LAYERS;
        int NUM_CLASSES;
        int L0_NEURONS;
        int L0_WORDS;       // words per neuron
        int L1_NEURONS;
        int L1_WORDS;
        int L2_NEURONS;
        int L2_WORDS;
        int L3_NEURONS;
        int L3_WORDS;
    } bnn_params_t;

    localparam bnn_params_t BNN_PARAMS = '{
        N_LAYERS:    4,
        NUM_CLASSES: 47,
        L0_NEURONS:  512,
        L0_WORDS:    13,
        L1_NEURONS:  512,
        L1_WORDS:    8,
        L2_NEURONS:  512,
        L2_WORDS:    8,
        L3_NEURONS:  47,
        L3_WORDS:    8
    };

endpackage
