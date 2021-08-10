// Minimal test with both, combinational and flip-flop
module top (
  input logic clk,
  input logic rst,
  input logic data,
  output logic result
);

  comb_ff_mixed u_comb_ff_mixed(clk, rst, data, result);

endmodule

module comb_ff_mixed (
  input logic clk,
  input logic rst,
  input logic data,
  output logic result
);

logic data_q;
  always_ff @(posedge clk or posedge rst) begin
    if (rst) begin
      data_q <= 1'b0;
    end else begin
      data_q <= data;
    end
  end

assign result = data & data_q;

endmodule
