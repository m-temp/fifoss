module top (
  input var logic clk,
  input var logic rst,
  input var logic count,
  output var logic done
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
      done <= 1'b0;
    end else begin
      if (finish_cnt >= 8'd8) begin
        done <= 1'b1;
      end else if (count) begin
        finish_cnt <= finish_cnt + 8'd1;
      end
    end
  end
endmodule
