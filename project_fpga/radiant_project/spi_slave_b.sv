// spi_slave.sv
// FPGA_B: receives character frames from FPGA_A over a 3-wire SPI link.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Link:
//   - 3 wires from FPGA_A: sck, mosi, cs_n (active low). No MISO.
//   - SPI Mode 0 (sample mosi on sck rising edge), LSB first (per MCU link).
//   - One frame per cs_n-low period: 2 bytes, [cmd][char]
//       cmd 0x01 = APPEND char to the log
//       cmd 0x02 = CLEAR the log (char ignored)
//   - Frames with any length other than 16 bits are silently discarded.
//
// Approach: oversample. sck, mosi, cs_n each pass through a 2-flop
// synchronizer into the 48 MHz clk domain; a third stage on sck and cs_n
// supports edge detection. 48 MHz / 5 MHz sck = ~10x oversampling.

module spi_slave (
    input  logic       clk,          // 48 MHz
    input  logic       rst_n,
    input  logic       sck,          // async from FPGA_A
    input  logic       mosi,
    input  logic       cs_n,
    output logic [7:0] cmd,
    output logic [7:0] data,
    output logic       frame_valid
);

    // 2-flop synchronizers + 3rd stage for SCK/CS edge detect
    logic sck_s0, sck_s1, sck_s2;
    logic mosi_s0, mosi_s1;
    logic cs_s0,  cs_s1,  cs_s2;

    always_ff @(posedge clk) begin
        sck_s0  <= sck;    sck_s1  <= sck_s0;   sck_s2  <= sck_s1;
        mosi_s0 <= mosi;   mosi_s1 <= mosi_s0;
        cs_s0   <= cs_n;   cs_s1   <= cs_s0;    cs_s2   <= cs_s1;
    end

    logic sck_rising, cs_rising;
    assign sck_rising = sck_s1 & ~sck_s2;
    assign cs_rising  = cs_s1  & ~cs_s2;     // frame end

    logic [15:0] shift_reg;
    logic [4:0]  bit_cnt;

    always_ff @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            shift_reg   <= '0;
            bit_cnt     <= '0;
            cmd         <= '0;
            data        <= '0;
            frame_valid <= 1'b0;
        end else begin
            frame_valid <= 1'b0;             // default: no pulse

            if (cs_rising) begin
                // frame just ended
                if (bit_cnt == 5'd16) begin
                    cmd         <= shift_reg[7:0];
                    data        <= shift_reg[15:8];
                    frame_valid <= 1'b1;
                end
                bit_cnt <= '0;
            end
            else if (!cs_s1 && sck_rising) begin
                shift_reg <= {mosi_s1, shift_reg[15:1]};
                bit_cnt   <= bit_cnt + 1'b1;
            end
        end
    end

endmodule