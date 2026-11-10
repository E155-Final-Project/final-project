module BNN_thresh (
    input  logic       clk,
    input  logic [1:0] layer,
    input  logic [8:0] neuron,
    output logic [9:0] thresh
);

    logic [9:0] l0_thresh, l1_thresh, l2_thresh;

    BNN_ebr_rom #(.DEPTH(512), .WIDTH(10), .MEM_FILE("l0_thresh.mem")) l0_thresh_rom (
        .clk(clk), .addr(neuron), .data(l0_thresh));

    BNN_ebr_rom #(.DEPTH(512), .WIDTH(10), .MEM_FILE("l1_thresh.mem")) l1_thresh_rom (
        .clk(clk), .addr(neuron), .data(l1_thresh));

    BNN_ebr_rom #(.DEPTH(512), .WIDTH(10), .MEM_FILE("l2_thresh.mem")) l2_thresh_rom (
        .clk(clk), .addr(neuron), .data(l2_thresh));

    always_comb
        case (layer)
            2'd0:    thresh = l0_thresh;
            2'd1:    thresh = l1_thresh;
            2'd2:    thresh = l2_thresh;
            default: thresh = '0;    // output layer uses argmax, not a threshold
        endcase

endmodule

module BNN_ebr_rom #(
    parameter int DEPTH    = 512,
    parameter int WIDTH    = 10,
    parameter     MEM_FILE = "l0_thresh.mem"
)(
    input  logic                     clk,
    input  logic [$clog2(DEPTH)-1:0] addr,
    output logic [WIDTH-1:0]         data
);

    logic [WIDTH-1:0] mem [0:DEPTH-1];

    initial $readmemh(MEM_FILE, mem);

    always_ff @(posedge clk)
        data <= mem[addr];

endmodule