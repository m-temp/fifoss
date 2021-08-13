module tb (
  input var logic clk,
  input var logic rst
);
  logic count;
  logic done;

  top u_top(clk, rst, count, done);

  assign count = 1'b1;

  always_ff @(posedge clk) begin
    if (done) begin
      $finish();
    end
  end
endmodule
