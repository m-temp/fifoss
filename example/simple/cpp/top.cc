#include <memory>
#include <iostream>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vtop.h"

int main(int argc, char *argv[], char **env) {

  const std::unique_ptr<VerilatedContext> cp{new VerilatedContext};

  cp->traceEverOn(true);

  cp->commandArgs(argc, argv);

  const std::unique_ptr<Vtop> top{new Vtop{cp.get(), "TOP"}};

  top->clk = 0;
  top->rst = 1;

  VerilatedVcdC *tfp = new VerilatedVcdC;
  top->trace(tfp, 99);
  tfp->open("trace.vcd");

  // Wait for top module to signal an end
  while (!cp->gotFinish()) {
    // Alternate clock
    cp->timeInc(1);
    top->clk = !top->clk;

    if (!top->clk) {
      if (cp->time() > 4) {
        top->rst = 0;
      }
    }
    top->eval();

    tfp->dump(cp->time());

    std::cout << cp->time() << ": " << (top->clk ? "1" : "0") << std::endl;
  }

  // Finish
  top->final();
  tfp->close();

  return 0;
}
