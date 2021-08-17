#ifndef FAULT_INJECTION_H_
#define FAULT_INJECTION_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <verilated.h>

struct fault {
	int temporal;
	int spatial;
};

namespace {
struct temporal {
	int start;
	int duration;
};
}

class FaultInjection {
  public:
    /**
     * Create fault injection by limiting the fault space.
     */
    FaultInjection(unsigned int fi_signal_len, struct temporal limit, bool mode_linear, unsigned long iteration_count = 0);

    /**
     * Create fault injection by specifying precise clock and position.
     */
    FaultInjection(unsigned int fi_signal_len, int fault_temporal, int fault_spatial);

    /**
     * Inject a fault if the configured criteria are met.
     *
     * Must be called each clock cycle.
     */
    template<typename T>
    bool UpdateInsert(T &fi);
    template<typename T>
    bool UpdateInsert(T *fi);

    /**
     * Check if a fault has been injected.
     */
    bool Injected();

    /**
     * Dump current fault configuration into output stream. Used for logging.
     */
    void SaveToLog(std::ofstream&);

    /**
     * Return the current fault configuration.
     */
    std::pair<int, int> GetFaultSpace();

  private:
    const unsigned int num_fi_signals;
    bool injected;
    unsigned long cycle_count;
    struct fault active_fault;
};

/* Fault injection for signals with a width < 65 */
template<typename T>
bool FaultInjection::UpdateInsert(T &fi_signal) {
  if (injected) {
    fi_signal = 0x00;
    return true;
  }
  if (cycle_count++ > active_fault.temporal) {
    std::cout << "Fault injection in cycle " << active_fault.temporal << " at number " << active_fault.spatial << " for one cycle" << std::endl;
    fi_signal = ((T) 0x1) << active_fault.spatial;
    injected = true;
    return true;
  }
  return false;
}

/* Fault injection for signals with a width > 64. Signals are handled as 32-bit arrays. */
template<typename T>
bool FaultInjection::UpdateInsert(T *fi_signal) {
  if (injected) {
    fi_signal[active_fault.spatial / 32] = 0x00000000;
    return true;
  }
  if (cycle_count++ > active_fault.temporal) {
    std::cout << "Fault injection in cycle " << active_fault.temporal << " at number " << active_fault.spatial << " for one cycle" << std::endl;
    fi_signal[active_fault.spatial / 32] = 0x1 << (active_fault.spatial % 32);
    injected = true;
    return true;
  }
  return false;
}

#endif // FAULT_INJECTION_H_

