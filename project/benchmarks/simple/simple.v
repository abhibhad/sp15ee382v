module simple (
inp1,
inp2,
iccad_clk,
out
);

// Start PIs
input inp1;
input inp2;
input iccad_clk;

// Start POs
output out;

// Start wires
wire n1;
wire n2;
wire n3;
wire n4;
wire inp1;
wire inp2;
wire iccad_clk;
wire out;

// Start cells
NAND2_X1 u1 ( .A1(inp1), .A2(inp2), .ZN(n1) );
DFF_X1 f1 ( .D(n2), .CK(iccad_clk), .Q(n3) );
INV_X1 u2 ( .A(n3), .ZN(n4) );
INV_X1 u3 ( .A(n4), .ZN(out) );
NOR2_X1 u4 ( .A1(n1), .A2(n3), .ZN(n2) );

endmodule
