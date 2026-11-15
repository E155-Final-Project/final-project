// Activation buffer for BNN_core: the input image and each hidden layer's output.
module Activation_EBR (
    input  logic        clk,

    // image load port (from the SPI receiver)
    input  logic        load_we,
    input  logic [3:0]  load_addr,     // input word 0..12
    input  logic [63:0] load_data,

    // from BNN_core
    input  logic [1:0]  layer,
    input  logic [4:0]  word,
    input  logic [9:0]  neuron,
    input  logic        act_bit,
    input  logic        act_we,

    output logic [63:0] activations
);

    logic rd_half, wr_half;
    assign rd_half = ~layer[0];   // even layers read top, odd layers read bottom
    assign wr_half =  layer[0];   // and write the other half

    // Write Port
    logic [3:0]  we;       // one per EBR block
    logic [7:0]  waddr;
    logic [15:0] mask;     // 1 = write this bit, shared by all four blocks
    logic [63:0] wdata;

    always_comb
        if (load_we) begin                       // whole input word
            we    = 4'b1111;
            waddr = {4'b1000, load_addr};
            mask  = 16'hFFFF;
            wdata = load_data;
        end
        else begin                               // one activation bit
            we    = act_we ? (4'b0001 << neuron[5:4]) : 4'b0000;
            waddr = {wr_half, 4'b0000, neuron[8:6]};
            mask  = 16'b1 << neuron[3:0];
            wdata = {64{act_bit}};
        end

    // Read Port
    logic [7:0] raddr;
    assign raddr = {rd_half, 3'b000, word[3:0]};

    // ---- the four EBR blocks ----
    // bits 15:0, 31:16, 47:32, 63:48
    EBR_256x16 ebr0 (
        .clk  (clk),
        .we   (we[0]),
        .waddr(waddr),
        .mask (mask),
        .wdata(wdata[15:0]),
        .raddr(raddr),
        .rdata(activations[15:0])
    );
    EBR_256x16 ebr1 (
        .clk  (clk),
        .we   (we[1]),
        .waddr(waddr),
        .mask (mask),
        .wdata(wdata[31:16]),
        .raddr(raddr),
        .rdata(activations[31:16])
    );
    EBR_256x16 ebr2 (
        .clk  (clk),
        .we   (we[2]),
        .waddr(waddr),
        .mask (mask),
        .wdata(wdata[47:32]),
        .raddr(raddr),
        .rdata(activations[47:32])
    );
    EBR_256x16 ebr3 (
        .clk  (clk),
        .we   (we[3]),
        .waddr(waddr),
        .mask (mask),
        .wdata(wdata[63:48]),
        .raddr(raddr),
        .rdata(activations[63:48])
    );

endmodule


// One iCE40 EBR block, instantiated directly as SB_RAM256x16 so it is sure to
// map to block RAM. Separate write and read ports, registered read.
// The mask on this wrapper is active high (1 = write this bit). SB_RAM256x16's
// MASK is the opposite (1 = don't write this bit), so it is inverted here.
module EBR_256x16 (
    input  logic        clk,
    input  logic        we,
    input  logic [7:0]  waddr,
    input  logic [15:0] mask,
    input  logic [15:0] wdata,
    input  logic [7:0]  raddr,
    output logic [15:0] rdata
);

    SB_RAM256x16 ram256x16_inst (
        .RDATA(rdata),
        .RADDR(raddr),
        .RCLK (clk),
        .RCLKE(1'b1),
        .RE   (1'b1),
        .WADDR(waddr),
        .WCLK (clk),
        .WCLKE(1'b1),
        .WDATA(wdata),
        .WE   (we),
        .MASK (~mask)
    );

endmodule
