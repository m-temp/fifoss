module top (
  input logic in_i,
  output logic out_o
);

  one u_one (
    .in_i(in_i),
    .out_o(out_o)
  );

endmodule

module one (
  input logic in_i,
  output logic out_o
);

  assign out_o = in_i & 1'b1;

endmodule
