#include <memory>
#include <iostream>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vtop.h"
#include "fault_injection.h"

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

  FaultInjection fi = FaultInjection(44, 10, 32);

  // Create a check for `alert_o` and delay the stop for 10 cycles
  fi.AddAbortWatch(&top->alert_o, 10);

  while (!top->done_o) {
    // Alternate clock
    cp->timeInc(1);
    top->clk = !top->clk;

    if (!top->clk) {
      if (cp->time() > 2) {
        top->rst = 0;
        top->start_i = true;
      }
    }

    if (top->clk) {
      fi.UpdateInsert(top->fi_combined);
    }

    top->eval();

    // Check for a stop request
    if (fi.StopRequested()) {
      std::cout << "Fault injection: Stop requested" << std::endl;
      break;
    }

    tfp->dump(cp->time());

  }

  // Finish
  top->final();
  tfp->close();

  return 0;
}
