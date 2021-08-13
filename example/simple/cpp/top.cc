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

  bool sim_done = false;
  while (!sim_done) {
    // Alternate clock
    cp->timeInc(1);
    top->clk = !top->clk;

    if (!top->clk) {
      if (cp->time() > 4) {
        top->rst = 0;
        top->count = true;
      }
    }
    top->eval();

    tfp->dump(cp->time());

    sim_done = top->done;
  }

  // Finish
  top->final();
  tfp->close();

  return 0;
}
