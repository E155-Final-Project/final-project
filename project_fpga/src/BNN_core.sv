module BNN_core(
	input logic clk, reset,
	output logic done
	);

	logic sum;
	logic [1:0] layer;
	logic [9:0] neuron_hidden;
	logic [4:0] word; 
	logic [6:0] neuron_out;
	
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
			// else
				// word = 0
				// if neuron_hidden < 512
					// neuron++
				// else
					// neuron = 0
					// if layer < 3
						// layer++
					// else
						// layer = 0
						// if neuron_out = 47
							// neuron_out ++
						// else
							// done = 1
					
						
		end
	
	end
	
	logic [63:0] data_in, weights;
	logic [63:0] data_xnor;
	assign data_xor = ~(data_in ^ weights);
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