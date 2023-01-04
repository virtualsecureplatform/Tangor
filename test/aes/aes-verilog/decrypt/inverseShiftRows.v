// https://github.com/michaelehab/AES-Verilog/blob/main/inverseShiftRows.v
module inverseShiftRows (
x,
z);

	input [127:0] x;
	output [127:0] z;
	
   // First row (r = 0) is not z
	assign z[0+:8] = x[0+:8];
	assign z[32+:8] = x[32+:8];
	assign z[64+:8] = x[64+:8];
   assign z[96+:8] = x[96+:8];
	
	// Second row (r = 1) is cyclically right z by 1 offset
   assign z[8+:8] = x[104+:8];
   assign z[40+:8] = x[8+:8];
   assign z[72+:8] = x[40+:8];
   assign z[104+:8] = x[72+:8];
	
	// Third row (r = 2) is cyclically right z by 2 offsets
   assign z[16+:8] = x[80+:8];
   assign z[48+:8] = x[112+:8];
   assign z[80+:8] = x[16+:8];
   assign z[112+:8] = x[48+:8];
	
	// Fourth row (r = 3) is cyclically right z by 3 offsets
   assign z[24+:8] = x[56+:8];
   assign z[56+:8] = x[88+:8];
   assign z[88+:8] = x[120+:8];
   assign z[120+:8] = x[24+:8];

endmodule
