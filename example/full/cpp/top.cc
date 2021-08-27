#include <fstream>
#include <memory>
#include <functional>
#include <iostream>
#include <string>

#include <verilated.h>
#include <verilated_vcd_c.h>

#include "Vtop.h"
#include "fault_injection.h"
#include "data_monitor.h"

int main(int argc, char *argv[], char **env) {

  std::ofstream fi_log;
  fi_log.open("fi_log.txt");

  const unsigned int fi_combined_len = 99;

  for (int i = 0; i < fi_combined_len*16; ++i) {
    const std::unique_ptr<VerilatedContext> cp{new VerilatedContext};

    cp->traceEverOn(true);

    cp->commandArgs(argc, argv);

    const std::unique_ptr<Vtop> top{new Vtop{cp.get(), "TOP"}};

    top->clk = 0;
    top->rst = 1;

    VerilatedVcdC *tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("trace.vcd");

    FaultInjection fi = FaultInjection(fi_combined_len);
    fi.SetModeRange(4, 20, true, i);
    fi.SetFaultDuration(2);

    // Create a check for `alert_o` and delay the stop for 10 cycles
    fi.AddAbortWatch("alert_o", &top->alert_o, 10);

    // Check for 8-bit signal
    // Define values to compare against
    CData data[] = {0xac, 0x57, 0x86};
    // Create object connected to a design signal and the comparison values
    DataMonitor<CData> data_o("data_o", &top->data_o, data, sizeof(data)/sizeof(CData));
    // Create a bind function
    std::function <bool (std::string &)> data_o_compare = std::bind(&DataMonitor<CData>::Compare, &data_o, std::placeholders::_1);
    // Add the function to the watch list
    fi.AddValueComparator(data_o_compare);

    // Check for 32-bit signal
    IData secret[] = {0xdeadbeef};
    DataMonitor<IData> secret_o("secret_o", &top->secret_o, secret, sizeof(secret)/sizeof(IData));
    std::function <bool (std::string &)> secret_o_compare = std::bind(&DataMonitor<IData>::Compare, &secret_o, std::placeholders::_1);
    fi.AddValueComparator(secret_o_compare);

    struct Fault f = fi.GetFaultSpace();
    std::cout << "Starting simulation with fault injection config: " << f << std::endl;

    while (cp->time() < 200) {
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
      if (top->clk) {
        if (fi.StopRequested()) {
          break;
        }
      }

      tfp->dump(cp->time());

    }

    fi_log << fi << std::endl;

    // Finish
    top->final();
    tfp->close();
  }

  fi_log.close();

  return 0;
}
