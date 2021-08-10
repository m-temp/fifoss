module top (
  input logic clk,
  input logic rst_n,
  input logic i,
  input logic en,
  output logic [3:0] o
);

  ffen u_ffen (clk, rst_n, i, en, o[0]);

  ffend u_ffend (clk, rst_n, i, en, o[1]);

  logic [7:0] tmp;
  logic [7:0] tmp_i;
  assign tmp_i = {7'b0101110, i};
  ff u_ff (clk, rst_n, tmp_i, tmp);

  assign o[2] = ^tmp;

  hidden u_hidden (clk, rst_n, i, en, o[3]);

endmodule

module hidden (
  input logic clk,
  input logic rst_n,
  input logic i,
  input logic en,
  output logic o
);

  logic [1:0] oo;
  assign o = ^oo;
  ffen_double u_ffen0 (clk, rst_n, i, en, oo[0]);
  ffen_double u_ffen1 (clk, rst_n, i, en, oo[1]);

endmodule

module ffen (
  input logic clk,
  input logic rst_n,
  input logic i,
  input logic en,
  output logic o
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      o <= '0;
    end else if (en) begin
      o <= i;
    end
  end
endmodule

module ffen_double (
  input logic clk,
  input logic rst_n,
  input logic i,
  input logic en,
  output logic o
);
  logic a, b;
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      a <= '0;
    end else if (en) begin
      a <= i;
    end
  end
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      b <= '0;
    end else if (en) begin
      b <= i;
    end
  end

  assign o = a ^ b;
endmodule

module ffend (
  input logic clk,
  input logic rst_n,
  input logic i,
  input logic en,
  output logic o
);
  logic d;
  always_comb begin
    d = (i & en) | (~en & o);
  end
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      o <= '0;
    end else begin
      o <= d;
    end
  end
endmodule

module ff (
  input logic clk,
  input logic rst_n,
  input logic [7:0] i,
  output logic [7:0] o
);
  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      o <= '0;
    end else begin
      o <= i;
    end
  end
endmodule
