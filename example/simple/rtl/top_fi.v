/* Generated by Yosys 0.9+4274 (git sha1 e6dd4db0a, clang 12.0.1 -fPIC -Os) */

module figenerator(fi_0, fi_1, fi_combined);
  output [9:0] fi_0;
  output [35:0] fi_1;
  input [45:0] fi_combined;
  assign fi_1 = fi_combined[45:10];
  assign fi_0 = fi_combined[9:0];
endmodule

(* top =  1  *)
(* src = "rtl/top.sv:1.1-29.10" *)
module top(clk, rst, count, done, fi_combined);
  (* src = "rtl/top.sv:9.3-11.6" *)
  wire _00_;
  (* src = "rtl/top.sv:17.3-28.6" *)
  wire _01_;
  (* src = "rtl/top.sv:17.3-28.6" *)
  wire [7:0] _02_;
  (* src = "rtl/top.sv:25.23-25.40" *)
  wire [7:0] _03_;
  wire [7:0] _04_;
  wire _05_;
  wire _06_;
  reg _07_;
  reg [7:0] _08_;
  reg _09_;
  wire [7:0] _10_;
  wire [7:0] _11_;
  wire _12_;
  wire _13_;
  wire [7:0] _14_;
  wire _15_;
  wire [7:0] _16_;
  wire _17_;
  wire [7:0] _18_;
  (* keep = 32'd1 *)
  (* src = "rtl/top.sv:7.18-7.19" *)
  wire a;
  (* src = "rtl/top.sv:2.19-2.22" *)
  input clk;
  (* src = "rtl/top.sv:4.19-4.24" *)
  input count;
  (* src = "rtl/top.sv:5.20-5.24" *)
  output done;
  wire [35:0] fi_comb;
  wire [7:0] fi_comb_0;
  wire fi_comb_1;
  wire [7:0] fi_comb_10;
  wire fi_comb_2;
  wire [7:0] fi_comb_6;
  wire [7:0] fi_comb_7;
  wire fi_comb_8;
  wire fi_comb_9;
  input [45:0] fi_combined;
  wire [9:0] fi_ff;
  wire fi_ff_3;
  wire [7:0] fi_ff_4;
  wire fi_ff_5;
  (* init = 8'h00 *)
  (* src = "rtl/top.sv:13.15-13.25" *)
  wire [7:0] finish_cnt;
  (* src = "rtl/top.sv:3.19-3.22" *)
  input rst;
  assign _04_ = finish_cnt + (* src = "rtl/top.sv:25.23-25.40" *) 8'h01;
  assign _03_ = fi_comb[7:0] ^ _04_;
  assign _15_ = fi_comb[8] ^ _05_;
  assign _00_ = fi_comb[9] ^ _06_;
  assign done = fi_ff[0] ^ _07_;
  assign finish_cnt = fi_ff[8:1] ^ _08_;
  assign a = fi_ff[9] ^ _09_;
  assign _16_ = fi_comb[17:10] ^ _10_;
  assign _02_ = fi_comb[25:18] ^ _11_;
  assign _17_ = fi_comb[26] ^ _12_;
  assign _01_ = fi_comb[27] ^ _13_;
  assign _18_ = fi_comb[35:28] ^ _14_;
  assign _05_ = finish_cnt >= (* src = "rtl/top.sv:22.11-22.29" *) 8'h08;
  assign _06_ = ~ (* src = "rtl/top.sv:10.10-10.12" *) a;
  (* \always_ff  = 32'd1 *)
  (* src = "rtl/top.sv:17.3-28.6" *)
  always @(posedge clk)
    _07_ <= _01_;
  (* \always_ff  = 32'd1 *)
  (* src = "rtl/top.sv:17.3-28.6" *)
  always @(posedge clk)
    _08_ <= _02_;
  (* \always_ff  = 32'd1 *)
  (* src = "rtl/top.sv:9.3-11.6" *)
  always @(posedge clk)
    _09_ <= _00_;
  assign _10_ = _15_ ? (* full_case = 32'd1 *) (* src = "rtl/top.sv:22.11-22.29|rtl/top.sv:22.7-26.10" *) finish_cnt : _18_;
  assign _11_ = rst ? (* full_case = 32'd1 *) (* src = "rtl/top.sv:18.9-18.12|rtl/top.sv:18.5-27.8" *) 8'h00 : _16_;
  assign _12_ = _15_ ? (* full_case = 32'd1 *) (* src = "rtl/top.sv:22.11-22.29|rtl/top.sv:22.7-26.10" *) 1'h1 : done;
  assign _13_ = rst ? (* full_case = 32'd1 *) (* src = "rtl/top.sv:18.9-18.12|rtl/top.sv:18.5-27.8" *) 1'h0 : _17_;
  assign _14_ = count ? (* src = "rtl/top.sv:24.20-24.25|rtl/top.sv:24.16-26.10" *) _03_ : finish_cnt;
  figenerator u_figenerator (
    .fi_0(fi_ff),
    .fi_1(fi_comb),
    .fi_combined(fi_combined)
  );
  assign fi_comb_8 = fi_comb[26];
  assign fi_comb_7 = fi_comb[25:18];
  assign fi_comb_6 = fi_comb[17:10];
  assign fi_comb_9 = fi_comb[27];
  assign fi_ff_5 = fi_ff[9];
  assign fi_ff_4 = fi_ff[8:1];
  assign fi_ff_3 = fi_ff[0];
  assign fi_comb_10 = fi_comb[35:28];
  assign fi_comb_2 = fi_comb[9];
  assign fi_comb_1 = fi_comb[8];
  assign fi_comb_0 = fi_comb[7:0];
endmodule
