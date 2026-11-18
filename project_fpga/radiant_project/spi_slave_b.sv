// spi_slave.sv
// FPGA_B: receives character frames from FPGA_A over a 3-wire SPI link.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Link (PROPOSED, settle before building):
//   - 3 wires from FPGA_A: sck, mosi, cs_n (active low). No MISO.
//   - SPI Mode 0 (sample mosi on sck rising edge), LSB first (per MCU link).
//   - One frame per cs_n-low period: 2 bytes, [cmd][char]
//       cmd 0x01 = APPEND char to the log
//       cmd 0x02 = CLEAR the log (char ignored)
//   - Frames with any length other than 16 bits are discarded.
//
// Approach: oversample. sck, mosi, cs_n each pass through a 2-flop
// synchronizer into the 48 MHz clk domain; detect sck rising edges there.
// 48 MHz / 5 MHz sck = ~10x oversampling, so no separate sck clock domain.
//
// TODO:
//   - 2-flop synchronizers on sck, mosi, cs_n
//   - sck rising-edge detect (sync'd sck, previous value)
//   - bit counter, reset when cs_n is high
//   - shift register: LSB first -> shift in at the top, shift right
//   - on cs_n rising edge: if exactly 16 bits, latch cmd/data, pulse frame_valid

module spi_slave (
    input  logic       clk,          // 48 MHz system clock (HSOSC)
    input  logic       rst_n,
    input  logic       sck,          // from FPGA_A, asynchronous to clk
    input  logic       mosi,
    input  logic       cs_n,
    output logic [7:0] cmd,          // valid when frame_valid pulses
    output logic [7:0] data,
    output logic       frame_valid   // 1-cycle pulse per complete frame
);

    // TODO

endmodule