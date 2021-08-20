#include <memory>
#include <iostream>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vtop.h"
#include "fault_injection.h"
#include "data_monitor.h"

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

  FaultInjection fi = FaultInjection(44, 13, 12);

  // Create a check for `alert_o` and delay the stop for 10 cycles
  fi.AddAbortWatch(&top->alert_o, 10);

  CData data[] = {0xac, 0x57, 0x86};
  DataMonitor<CData> data_o(&top->data_o, data, sizeof(data)/sizeof(CData));
  std::function <bool ()> data_o_compare = std::bind(&DataMonitor<CData>::Compare, &data_o);

  IData secret[] = {0xdeadbeef};
  DataMonitor<IData> secret_o(&top->secret_o, secret, sizeof(secret)/sizeof(IData));
  std::function <bool ()> secret_o_compare = std::bind(&DataMonitor<IData>::Compare, &secret_o);

  fi.AddValueComparator(data_o_compare);
  fi.AddValueComparator(secret_o_compare);


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
