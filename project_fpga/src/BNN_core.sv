module BNN_core #(parameter BNN_params::bnn_params_t P = BNN_params::BNN_PARAMS) (
	input logic clk, reset,		// control signals
	output logic done,
	
	input logic [63:0] data_in, weight,
	input logic  [9:0] thresh,
	output logic [4:0]  word,
	output logic [1:0]  layer,
	output logic [9:0]  neuron,
	output logic [13:0] weight_addr,
	output logic [7:0]  class_out,
	output logic        act_bit, act_we
	);

	// Layer Calculation
	logic [63:0] data_xnor;
	logic [6:0]  pop;
	assign data_xnor = ~(data_in ^ weight);
	popcount popcount1(.data_in(data_xnor), .count_out(pop));

	// Control Signals
	localparam logic [0:P.N_LAYERS-1][4:0] N_WORD   = '{P.L0_WORDS, P.L1_WORDS, P.L2_WORDS, P.L3_WORDS};
	localparam logic [0:P.N_LAYERS-1][9:0] N_NEURON = '{P.L0_NEURONS, P.L1_NEURONS, P.L2_NEURONS, P.L3_NEURONS};
	logic  last_word, last_neuron, last_layer;
	assign last_word   = (word == N_WORD[layer]);
	assign last_neuron = (neuron == N_NEURON[layer] - 1);
	assign last_layer  = (layer == P.N_LAYERS - 1);

	// Popcount sums
	logic [9:0]  sum, sum_next, best;
	assign sum_next = sum + pop;
	assign act_bit = (sum_next >= thresh);          // only valid on the last word
	assign act_we  = last_word & ~last_layer & ~done;

	always_ff @(posedge clk) begin
		if (reset) begin
			word <= 0;
			weight_addr <= 0;
			neuron <= 0;
			layer <= 0;
			sum <= 0; best <= 0; class_out <= 0; done <= 0;
		end
		else if (!done) begin // cycle through words
			if (!last_word) begin
				word        <= word + 1;
				weight_addr <= weight_addr + 1;
				if (word != 0) sum <= sum_next;   // data for word-1 arrives now
			end
			else begin // end of neuron
				word <= 0;
				sum  <= 0;
				if (last_layer && sum_next > best) begin 
					best      <= sum_next;
					class_out <= neuron[7:0];
				end
				if (!last_neuron)
					neuron <= neuron + 1;
				else begin // end of layer
					neuron <= 0;
					if (last_layer) done  <= 1;
					else            layer <= layer + 1;
				end
			end
		end
	end
	

endmodule