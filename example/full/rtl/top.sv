module top (
  input var logic clk,
  input var logic rst,
  input var logic start_i,
  output var logic alert_o,
  output var logic done_o
);
  typedef enum logic [2:0] {
    IDLE  = 3'b000,
    COUNT = 3'b001,
    TWO   = 3'b010,
    ERROR = 3'b110,
    FINAL = 3'b111
  } state_e;

  state_e state_ns, state_cs;

  logic [3:0] cnt;
  logic start_count;

  always_comb begin : fsm
    alert_o     = 1'b0;
    done_o      = 1'b0;
    start_count = 1'b0;

    state_ns = state_cs;
    case (state_ns)
      IDLE: begin
        if (start_i) begin
          state_ns = COUNT;
        end
      end

      COUNT: begin
        start_count = 1'b1;
        if (cnt == 4'b1100) begin
          state_ns = TWO;
        end
      end

      TWO: begin
        state_ns = FINAL;
        done_o   = 1'b1;
      end

      FINAL: begin
        state_ns = FINAL;
      end

      ERROR: begin
        alert_o = 1'b1;
      end

      default: begin
        state_ns = ERROR;
      end
    endcase
  end : fsm

  always_ff @(posedge clk, posedge rst) begin : state
    if (rst) begin
      state_cs = IDLE;
    end else begin
      state_cs = state_ns;
    end
  end : state

  always_ff @(posedge clk, posedge rst) begin : counter
    if (rst) begin
      cnt = 4'b0;
    end else begin
      if (start_count) begin
        cnt = cnt + 4'b1;
      end else begin
        cnt = 4'b0;
      end
    end
  end : counter

endmodule
