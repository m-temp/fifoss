module top (
  input logic [3:0] in_i,
  output logic [3:0] out_o
);
  simple u_sim_one (
    .in_i(in_i[1:0]),
    .out_o(out_o[1:0])
  );
  simple u_sim_two (
    .in_i(in_i[3:2]),
    .out_o(out_o[3:2])
  );

  assign out_o[2] = in_i[2] | in_i[3];
  assign out_o[3] = in_i[2] ^ ~in_i[3];
endmodule

module simple (
  input logic [1:0] in_i,
  output logic [1:0] out_o
);
  third u_third(.in_i, .out_o);
endmodule

module third (
  input logic [1:0] in_i,
  output logic [1:0] out_o
);
  assign out_o[0] = in_i[0] & in_i[1];
  assign out_o[1] = in_i[0] ^ in_i[1];
endmodule
