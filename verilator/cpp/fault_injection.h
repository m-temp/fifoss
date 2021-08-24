#ifndef FAULT_INJECTION_H_
#define FAULT_INJECTION_H_

#include <cstdint>
#include <fstream>
#include <functional>
#include <ostream>
#include <vector>
#include <sstream>
#include <string>
#include <verilated.h>

struct Fault {
  int temporal;
  int spatial;
  friend std::ostream &operator<<(std::ostream& os, const struct Fault& f);
};

struct AbortInfo {
  const std::string name_;
  CData *signal;
  bool positive_polarity;
  const unsigned int delay;
  unsigned int delay_count;
  bool asserted;
};

struct Temporal {
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

    friend std::ostream &operator<<(std::ostream& os, const FaultInjection& f);

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
    void AddAbortWatch(const char *name, CData *signal, unsigned int delay = 0, bool positive_polarity = true);

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
    void AddValueComparator(std::function<bool (std::string &s)>&);

    /**
     * Set the duration of an active fault.
     *
     * Number of cycles in which the fault is active (asserted), default is one cycle.
     */
    void SetFaultDuration(unsigned int length) {injection_duration_= length;};

    /**
     * Check if a fault has been injected.
     */
    bool Injected();

    /**
     * Dump current fault configuration into output stream. Used for logging.
     */
    void DumpConfig(std::ofstream&);

    /**
     * Return the current fault configuration.
     */
    struct Fault GetFaultSpace();

  private:
    const unsigned int num_fi_signals;
    bool injected_;
    unsigned int injection_duration_;
    unsigned long cycle_count_;
    struct Fault active_fault_;
    std::vector<struct AbortInfo> abort_watch_list_;
    std::vector<std::function<bool (std::string &)>> value_compare_list_;
    std::stringstream log_;
};

// TODO: make fault active length variable

/* Fault injection for signals with a width < 65 */
template<typename T>
bool FaultInjection::UpdateInsert(T &fi_signal) {
  cycle_count_++;
  if (injected_) {
    if (injection_duration_>1) {
      injection_duration_--;
    } else {
      fi_signal = 0x00;
      return true;
    }
  }
  if (cycle_count_ >= active_fault_.temporal) {
    fi_signal = ((T) 0x1) << active_fault_.spatial;
    injected_ = true;
    log_ << cycle_count_ << "\t" << active_fault_ << "\t" << "Fault inserted" << std::endl;
    return true;
  }
  return false;
}

/* Fault injection for signals with a width > 64. Signals are handled as 32-bit arrays. */
template<typename T>
bool FaultInjection::UpdateInsert(T *fi_signal) {
  cycle_count_++;
  if (injected_) {
    if (injection_duration_>1) {
      injection_duration_--;
    } else {
      fi_signal[active_fault_.spatial / 32] = 0x00000000;
      return true;
    }
  }
  if (cycle_count_ >= active_fault_.temporal) {
    fi_signal[active_fault_.spatial / 32] = 0x1 << (active_fault_.spatial % 32);
    injected_ = true;
    log_ << cycle_count_ << "\t" << active_fault_ << "\t" << "Fault inserted" << std::endl;
    return true;
  }
  return false;
}

#endif // FAULT_INJECTION_H_
