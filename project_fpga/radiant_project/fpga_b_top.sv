// fpga_b_top.sv
// FPGA_B top level: SPI link in, 16 x 14-segment rolling log out.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Data path:
//   spi_slave -> (cmd decode) -> char_buffer -> seg_rom -> scan_fsm -> pins
//                                     ^                       |
//                                     +------ digit_idx ------+
//
// Pins: 3 in (sck, mosi, cs_n) + 15 segment + 16 digit = 34 of ~39.
//   Pin assignments go in the .pcf. TODO: assign once the board is wired.
//
// Clock: SB_HFOSC at 48 MHz (no PLL needed on FPGA_B).
// Reset: power-on reset counter (a few cycles after configuration).
//
// DEMO_MODE = 1: ignore SPI; append the next character of a test string every
// 0.5 s. Proves the driver board, segment map and brightness before FPGA_A
// exists.
//
// TODO:
//   - SB_HFOSC instance, power-on reset
//   - cmd decode: 0x01 -> append, 0x02 -> clear (PROPOSED)
//   - DEMO_MODE character generator
//   - instantiate spi_slave, char_buffer, seg_rom, scan_fsm

module fpga_b_top #(
    parameter bit DEMO_MODE = 0
) (
    input  logic        sck,         // from FPGA_A
    input  logic        mosi,
    input  logic        cs_n,
    output logic [14:0] seg,         // segment bus, to driver board
    output logic [15:0] dig          // digit selects, to driver board
);

    // TODO

endmodule