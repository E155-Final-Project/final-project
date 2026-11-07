module popcount #(
    parameter WIDTH = 64
)(
    input  [WIDTH-1:0] data_in,
    output logic [$clog2(WIDTH):0] count_out
);

    always_comb begin
        count_out = $countones(data_in);
    end

endmodule