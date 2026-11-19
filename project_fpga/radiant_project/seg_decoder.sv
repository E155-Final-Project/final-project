// seg_decoder.sv
// FPGA_B: ASCII character -> 14-segment + decimal point pattern.
//
// Author: Haspard Murphy   Created: 2026-11-11
//
// Output: 15 bits, 1 = segment lit (logical; polarity handled in scan_fsm).
//   Bit map:
//     [0]=A  [1]=B  [2]=C  [3]=D  [4]=E  [5]=F  [6]=G1 [7]=G2
//     [8]=H  [9]=J  [10]=K [11]=L [12]=M [13]=N [14]=DP
//
// Characters: the 47 EMNIST Balanced classes (0-9, A-Z, a b d e f g h n q r t)
//   + space (8'h20) = all off. Any other byte -> all segments on, loud enough
//   to spot an un-mapped class at a glance.
//
// Several glyphs are physically identical on 14 segments (0/O, 5/S, 8/B, 9/g,
// F/f) and are left ambiguous by design.

module seg_rom (
    input  logic [7:0]  ascii,
    output logic [14:0] segs);        // 14 segments + DP, 1 = on

    always_comb
        case (ascii)
            // Digits
            "0": segs = 15'h003F;  // A B C D E F
            "1": segs = 15'h0006;  // B C
            "2": segs = 15'h00DB;  // A B D E G1 G2
            "3": segs = 15'h00CF;  // A B C D G1 G2
            "4": segs = 15'h00E6;  // B C F G1 G2
            "5": segs = 15'h00ED;  // A C D F G1 G2
            "6": segs = 15'h00FD;  // A C D E F G1 G2
            "7": segs = 15'h0007;  // A B C
            "8": segs = 15'h00FF;  // A B C D E F G1 G2
            "9": segs = 15'h00EF;  // A B C D F G1 G2

            // Uppercase
            "A": segs = 15'h00F7;  // A B C E F G1 G2
            "B": segs = 15'h00FF;  // == 8
            "C": segs = 15'h0039;  // A D E F
            "D": segs = 15'h121E;  // B C D E J M
            "E": segs = 15'h00F9;  // A D E F G1 G2
            "F": segs = 15'h00F1;  // A E F G1 G2
            "G": segs = 15'h00BD;  // A C D E F G2
            "H": segs = 15'h00F6;  // B C E F G1 G2
            "I": segs = 15'h1200;  // J M
            "J": segs = 15'h001E;  // B C D E
            "K": segs = 15'h0C70;  // E F G1 K L
            "L": segs = 15'h0038;  // D E F
            "M": segs = 15'h0536;  // B C E F H K
            "N": segs = 15'h0936;  // B C E F H L
            "O": segs = 15'h003F;  // == 0
            "P": segs = 15'h00F3;  // A B E F G1 G2
            "Q": segs = 15'h083F;  // A B C D E F L
            "R": segs = 15'h08F3;  // A B E F G1 G2 L
            "S": segs = 15'h00ED;  // == 5
            "T": segs = 15'h1201;  // A J M
            "U": segs = 15'h003E;  // B C D E F
            "V": segs = 15'h2830;  // E F L N
            "W": segs = 15'h3836;  // B C E F L M N
            "X": segs = 15'h2D00;  // H K L N
            "Y": segs = 15'h1500;  // H K M
            "Z": segs = 15'h2409;  // A D K N

            // Lowercase (EMNIST Balanced subset)
            "a": segs = 15'h00DC;  // C D E G1 G2
            "b": segs = 15'h00FC;  // C D E F G1 G2
            "d": segs = 15'h00DE;  // B C D E G1 G2
            "e": segs = 15'h00FB;  // A B D E F G1 G2
            "f": segs = 15'h00F1;  // == F
            "g": segs = 15'h00EF;  // == 9
            "h": segs = 15'h00F4;  // C E F G1 G2
            "n": segs = 15'h00D4;  // C E G1 G2
            "q": segs = 15'h20E7;  // A B C F G1 G2 N
            "r": segs = 15'h0070;  // E F G1
            "t": segs = 15'h00F8;  // D E F G1 G2

            " ": segs = 15'h0000;

            default: segs = 15'h7FFF;  // unknown byte: all segments on
        endcase

endmodule
