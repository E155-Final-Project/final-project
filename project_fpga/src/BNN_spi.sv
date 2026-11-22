module BNN_spi(input  logic        sck,
               input  logic        sdi,
               input  logic        load,
               output logic        sdo,
               input  logic        done,
               output logic [63:0] word,       // last finished 64-bit chunk
               output logic        word_half,  // high for bits 32-63 of a word; falls when word updates
               input  logic [7:0]  class_out);

    logic [5:0]  bitcnt;                // bit counter
    logic [63:0] shreg;                 // spi shift register
    logic        sdodelayed, wasdone;   
    logic [7:0]  resultcaptured;        // class from BNN core
    logic        started, ink;          // ink: frame had at least one 1 bit

    // How a transaction works:
    //   1. The MCU raises load (and load_weights too, if it is sending weights).
    //   2. The MCU sends the data, LSB first.
    //   3. Every 64 bits make one word. The word goes to the weights or the image.
    //   4. The MCU drops load. The BNN starts.
    //   5. When the BNN finishes, done goes high.
    //   6. The MCU sends 8 more clocks and reads back the class, LSB first.
    // SPI mode 0: data is sampled on the rising edge of sck

    // count bits while load is high, wraps every 64
    always_ff @(posedge sck or negedge load)
        if (!load) bitcnt <= 0;
        else       bitcnt <= bitcnt + 6'd1;

    // shift in at the top, so after 64 bits the first bit is in bit 0
    always_ff @(posedge sck) begin
        shreg <= {sdi, shreg[63:1]};
        if (load && bitcnt == 6'd63) begin
            word <= {sdi, shreg[63:1]};
        end
    end

    // bitcnt[5] signals the memory to take the new last word
    assign word_half = bitcnt[5];

    // blank frame check
    always_ff @(posedge sck or negedge load)
        if (!load) started <= 1'b0;
        else       started <= 1'b1;
    always_ff @(posedge sck)
        if (load) ink <= (started & ink) | sdi;

    // class out, LSB first; a blank frame sends all 1's
    always_ff @(posedge sck)
        if (!wasdone) resultcaptured <= ink ? class_out : 8'hFF; // if no data, output all 1's
        else          resultcaptured <= {1'b0, resultcaptured[7:1]};

    // sdo should change on the negative edge of sck
    always_ff @(negedge sck) begin
        wasdone    <= done;
        sdodelayed <= resultcaptured[1];
    end

    // when done is first asserted, shift out the lsb before the clock edge
    assign sdo = (done & !wasdone) ? (class_out[0] | ~ink) : sdodelayed;
endmodule
