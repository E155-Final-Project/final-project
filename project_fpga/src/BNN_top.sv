module BNN_top #(parameter BNN_params::bnn_params_t P = BNN_params::BNN_PARAMS) (
	input  logic sck,
	input  logic sdi,
	output logic sdo,
	input  logic load,
	input  logic load_weights,	// 1: SPI words go to the weights, 0: to the image
	output logic done
	);


	// declare an internal clock of 48 Mhz
	logic clk;
	HSOSC #(.CLKHF_DIV(2'b00)) hf_osc (.CLKHFPU(1'b1), .CLKHFEN(1'b1), .CLKHF(clk));

	// SPI: recieves 64-bit words, sends back the class
	logic [63:0] spi_word;
	logic        word_tog;
	logic [7:0]  class_out;

	BNN_spi BNN_spi(
		.sck(sck), .sdi(sdi), .load(load), .sdo(sdo), .done(done),
		.word(spi_word), .word_tog(word_tog), .class_out(class_out));

	// bring word_tog, load and load_weights into clk
	logic [2:0] tog_s, load_s;
	logic [1:0] lw_s;
	always_ff @(posedge clk) begin
		tog_s  <= {tog_s[1:0],  word_tog};
		load_s <= {load_s[1:0], load};
		lw_s   <= {lw_s[0],     load_weights};
	end

	logic new_word, load_start;
	assign new_word   = tog_s[2] ^ tog_s[1];		// one clk pulse per 64-bit word
	assign load_start = load_s[1] & ~load_s[2];		// one clk pulse when load rises

	// core runs once a transfer finishes, and is held in reset during one
	logic core_reset;
	assign core_reset = load_s[1] | lw_s[1];

	// core <-> memory signals
	logic [63:0] data_in, weight;
	logic  [9:0] thresh;
	logic  [4:0] word_idx;
	logic  [1:0] layer;
	logic  [9:0] neuron;
	logic [13:0] weight_addr;
	logic        act_bit, act_we;

	memory #(.P(P)) memory(
		.clk(clk),
		.new_word(new_word), .load_start(load_start), .load_weights(lw_s[1]), .load_data(spi_word),
		.weight_addr(weight_addr), .layer(layer), .word(word_idx), .neuron(neuron),
		.act_bit(act_bit), .act_we(act_we),
		.weight(weight), .thresh(thresh), .activations(data_in));

	BNN_core #(.P(P)) BNN_core(
		.clk(clk), .reset(core_reset), .done(done),
		.data_in(data_in), .weight(weight), .thresh(thresh),
		.word(word_idx), .layer(layer), .neuron(neuron),
		.weight_addr(weight_addr), .class_out(class_out),
		.act_bit(act_bit), .act_we(act_we));

endmodule
