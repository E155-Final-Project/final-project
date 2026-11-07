`include "BNN_params.svh"

module BNN_top(
	input  logic sck, 
	input  logic sdi,
	output logic sdo,
	input  logic load,
	output logic done
	);


	// declare an internal clock of 48 Mhz
	logic int_clk;
	HSOSC #(.CLKHF_DIV(2'b00)) hf_osc (.CLKHFPU(1'b1), .CLKHFEN(1'b1), .CLKHF(int_clk));
	
	// BNN_spi BNN_spi();	// recieves all the data
	// BNN_core BNN_core(); // does all the calculations
	
endmodule