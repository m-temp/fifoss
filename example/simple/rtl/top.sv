module top (
  input var logic clk,
  input var logic rst

);
  (*keep*) logic a;

  always_ff @(posedge clk) begin
    a <= ~a;
  end

  logic [7:0] finish_cnt;
  initial
    finish_cnt = 0;

  always_ff @(posedge clk) begin
    if (rst) begin
      finish_cnt <= '0;
    end else begin
      if (finish_cnt >= 8) begin
        $finish();
      end else begin
        finish_cnt <= finish_cnt + 1;
      end
    end
  end
endmodule
