// http://cs-www.cs.yale.edu/homes/peralta/CircuitStuff/SLP_AES_113.txt
module sbox(
	output [7:0] SubByte,
	input [7:0] num
	);
	
	wire[0:7] S, U;

	assign U = num;
	assign SubByte = S;
	
assign y14 = U[3] ^ U[5];
assign y13 = U[0] ^ U[6];
assign y9 = U[0] ^ U[3];
assign y8 = U[0] ^ U[5];
assign t0 = U[1] ^ U[2];
assign y1 = t0 ^ U[7];
assign y4 = y1 ^ U[3];
assign y12 = y13 ^ y14;
assign y2 = y1 ^ U[0];
assign y5 = y1 ^ U[6];
assign y3 = y5 ^ y8;
assign t1 = U[4] ^ y12;
assign y15 = t1 ^ U[5];
assign y20 = t1 ^ U[1];
assign y6 = y15 ^ U[7];
assign y10 = y15 ^ t0;
assign y11 = y20 ^ y9;
assign y7 = U[7] ^ y11;
assign y17 = y10 ^ y11;
assign y19 = y10 ^ y8;
assign y16 = t0 ^ y11;
assign y21 = y13 ^ y16;
assign y18 = U[0] ^ y16;
assign t2 = y12 & y15;
assign t3 = y3 & y6;
assign t4 = t3 ^ t2;
assign t5 = y4 & U[7];
assign t6 = t5 ^ t2;
assign t7 = y13 & y16;
assign t8 = y5 & y1;
assign t9 = t8 ^ t7;
assign t10 = y2 & y7;
assign t11 = t10 ^ t7;
assign t12 = y9 & y11;
assign t13 = y14 & y17;
assign t14 = t13 ^ t12;
assign t15 = y8 & y10;
assign t16 = t15 ^ t12;
assign t17 = t4 ^ y20;
assign t18 = t6 ^ t16;
assign t19 = t9 ^ t14;
assign t20 = t11 ^ t16;
assign t21 = t17 ^ t14;
assign t22 = t18 ^ y19;
assign t23 = t19 ^ y21;
assign t24 = t20 ^ y18;
assign t25 = t21 ^ t22;
assign t26 = t21 & t23;
assign t27 = t24 ^ t26;
assign t28 = t25 & t27;
assign t29 = t28 ^ t22;
assign t30 = t23 ^ t24;
assign t31 = t22 ^ t26;
assign t32 = t31 & t30;
assign t33 = t32 ^ t24;
assign t34 = t23 ^ t33;
assign t35 = t27 ^ t33;
assign t36 = t24 & t35;
assign t37 = t36 ^ t34;
assign t38 = t27 ^ t36;
assign t39 = t29 & t38;
assign t40 = t25 ^ t39;
assign t41 = t40 ^ t37;
assign t42 = t29 ^ t33;
assign t43 = t29 ^ t40;
assign t44 = t33 ^ t37;
assign t45 = t42 ^ t41;
assign z0 = t44 & y15;
assign z1 = t37 & y6;
assign z2 = t33 & U[7];
assign z3 = t43 & y16;
assign z4 = t40 & y1;
assign z5 = t29 & y7;
assign z6 = t42 & y11;
assign z7 = t45 & y17;
assign z8 = t41 & y10;
assign z9 = t44 & y12;
assign z10 = t37 & y3;
assign z11 = t33 & y4;
assign z12 = t43 & y13;
assign z13 = t40 & y5;
assign z14 = t29 & y2;
assign z15 = t42 & y9;
assign z16 = t45 & y14;
assign z17 = t41 & y8;
assign tc1 = z15 ^ z16;
assign tc2 = z10 ^ tc1;
assign tc3 = z9 ^ tc2;
assign tc4 = z0 ^ z2;
assign tc5 = z1 ^ z0;
assign tc6 = z3 ^ z4;
assign tc7 = z12 ^ tc4;
assign tc8 = z7 ^ tc6;
assign tc9 = z8 ^ tc7;
assign tc10 = tc8 ^ tc9;
assign tc11 = tc6 ^ tc5;
assign tc12 = z3 ^ z5;
assign tc13 = z13 ^ tc1;
assign tc14 = tc4 ^ tc12;
assign S[3] = tc3 ^ tc11;
assign tc16 = z6 ^ tc8;
assign tc17 = z14 ^ tc10;
assign tc18 = ~tc13 ^ tc14;
assign S[7] = z12 ^ tc18;
assign tc20 = z15 ^ tc16;
assign tc21 = tc2 ^ z11;
assign S[0] = tc3 ^ tc16;
assign S[6] = tc10 ^ tc18;
assign S[4] = tc14 ^ S[3];
assign S[1] = ~(S[3] ^ tc16);
assign tc26 = tc17 ^ tc20;
assign S[2] = ~(tc26 ^ z17);
assign S[5] = tc21 ^ tc17;
  
endmodule 

module SubBytes(
x,
z);

  input [127:0] x;
  output [127:0] z;

  generate
    genvar i;
    for (i = 0; i < 16; i = i + 1) begin:SBOX
	  sbox sbox(z[8*(i+1)-1:8*i], x[8*(i+1)-1:8*i]);
    end
  endgenerate


endmodule

