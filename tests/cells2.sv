module top (
  input logic [1:0] in_i,
  output logic [1:0] out_o
);

  two u_two (
    .in_i(in_i),
    .out_o(out_o)
  );

endmodule

module two (
  input logic [1:0] in_i,
  output logic [1:0] out_o
);

  assign out_o[0] = in_i[0] & in_i[1];
  assign out_o[1] = in_i[0] ^ in_i[1];

endmodule
