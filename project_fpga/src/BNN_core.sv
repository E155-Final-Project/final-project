module BNN_core(
	input logic clk, reset,		// control signals
	output logic done,
	
	input logic [63:0] data_in, weight, thresh,
	output logic [4:0] word,
	output logic [1:0] layer,
	output logic [9:0] neuron_hidden,
	output logic [6:0] neuron_out
	
	);

	logic sum;

	logic [N_LAYERS-1:0][4:0] N_word = '{L0_WORDS, L1_WORDS, L2_WORDS, L3_WORDS};
	logic [N_LAYERS-1:0][9:0] N_neuron = '{L0_WORDS, L1_WORDS, L2_WORDS, L3_WORDS};
	
	always_ff @(posedge clk) begin
		if (reset) begin
			layer <= 0;
			neuron_hidden <= 0;
			word <= 0;
			neuron_out <= 0;
			done <= 0;
			end
		else begin
			// if word < maxword[layer] // (13 / 5)
				// word++
				// sum += popcount
			// else
				// word = 0
				// neuron =  (sum >= thresh)
				// sum = 0
				// if neuron_hidden < Neurons[layer]
					// neuron++
				// else
					// neuron = 0
					// if layer < (N_Layers - 1)
						// layer++
					// else
						// layer = 0
						// if neuron_out = NUM_CLASSES
							// neuron_out ++
						// else
							// done = 1
					
						
		end
	
	end
	
	logic [63:0] data_xnor;
	assign data_xor = ~(data_in ^ weight);
	logic [9:0] popcount;
	popcount popcount1(.data_in(data_xor), .count_out(popcount));
	



endmodule

	//for each layer L:
		//for neuron n in 0 .. N_OUT-1:
			//sum = 0
			//for k in 0 .. WORDS-1:
				//w = weights[L][n*WORDS + k]
				//sum += popcount(~(act[k] ^ w))
			//if L is hidden:
				//next_act[n/64] bit (n%64) = (sum >= thresh[L][n])
			//else if sum > best:          // strict >, so the lowest class wins ties
				//best = sum; class = n
		//act = next_act