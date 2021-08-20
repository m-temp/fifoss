#ifndef FAULT_INJECTION_H_
#define FAULT_INJECTION_H_

#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>
#include <verilated.h>

struct fault {
  int temporal;
  int spatial;
};

struct abortWatch {
  CData *signal;
  bool positive_polarity;
  unsigned int delay;
  bool asserted;
};

struct temporal {
  int start;
  int duration;
};

class FaultInjection {
  public:
    /**
     * Create fault injection by limiting the fault space.
     */
    FaultInjection(unsigned int fi_signal_len, int temporal_start, int temporal_duration, bool mode_linear, unsigned long int iteration_count = 0);

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
     * Add an abort signal to the watch list.
     *
     * Abort signals are signals which, when asserted, are meant so signal an
     * end of the simulation.
     * This can be for example an alter handler which is triggered by an
     * injected fault.
     */
    void AddAbortWatch(CData *signal, unsigned int delay = 0, bool positive_polarity = true);

    /**
     * Convey a request to stop the simulation.
     *
     * This is triggered by an assertion of an abort signal, see `AddAbortWatch`, and
     * by a positive data comparsion from the values added by `AddValueComparator`.
     */
    bool StopRequested(void);

    /**
     * Add a design source and values to check against each cycle.
     *
     * Values are compared in `StopRequested`.
     */
    void AddValueComparator(std::function<bool ()>&);

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
    std::vector<struct abortWatch> abort_watch;
    std::vector<std::function<bool ()>> monitor_list;
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

