#include <functional>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>
#include <utility>
#include "fault_injection.h"

FaultInjection::FaultInjection(unsigned int fi_signal_len)
  : num_fi_signals(fi_signal_len),
    injected_(false),
    injection_duration_(1),
    cycle_count_(0) {
    // Set default values
    active_fault_ = Fault {1, 1};
}

void FaultInjection::SetModeRange(unsigned int temporal_start, unsigned int temporal_duration, bool mode_linear, unsigned long int iteration_count) {
  struct Temporal temporal_limit = Temporal{temporal_start, temporal_duration};

  // Two different ways to set the fault for a specific run.
  if (mode_linear) {
    // Linear mode need the current iteration number and will then iterate over the space.
    // Low frequency for clock and high frequency for position.
    active_fault_.temporal = iteration_count / num_fi_signals + temporal_limit.start;
    active_fault_.spatial = iteration_count % num_fi_signals;
  } else {
    // Choose values randomly
    // TODO: Allow to set a manual seed or at some other randomness.
    active_fault_.temporal = rand() % temporal_limit.start + temporal_limit.duration;
    active_fault_.spatial = rand() % (num_fi_signals + 1);
  }
  log_ << "Fault injection configured with:\n\tfault signal width: " << num_fi_signals << "\n\tfault cycle: " << active_fault_.temporal
    << " (" << temporal_start << "-" << temporal_duration << ")"
    << "\n\tfault signal number: " << active_fault_.spatial << std::endl;
}

void FaultInjection::SetModePrecise(unsigned int fault_temporal, unsigned int fault_spatial) {
  active_fault_ = Fault {fault_temporal, fault_spatial};
  log_ << "Fault injection configured with:\nfault signal width: " << num_fi_signals << "\nfault cycle: " << active_fault_.temporal
    << "\nfault signal number: " << active_fault_.spatial << std::endl;
}

struct Fault FaultInjection::GetFaultSpace() {
  return active_fault_;
}

bool FaultInjection::Injected() {
  return injected_;
}

void FaultInjection::DumpConfig(std::ofstream &olog) {
      olog << active_fault_ << std::endl;
}

void FaultInjection::AddAbortWatch(const char *name, CData *signal, unsigned int delay, bool positive_polarity) {
  abort_watch_list_.push_back(AbortInfo{name, signal, positive_polarity, delay, delay, false});
}

bool FaultInjection::StopRequested() {
  // Check for an abort signal
  for (auto a = abort_watch_list_.begin(); a != abort_watch_list_.end(); ++a) {
    // Store a signal assertion
    if (*a->signal == a->positive_polarity) {
      a->asserted = true;
    }
    if (a->asserted) {
      if (a->delay == a->delay_count) {
        log_ << cycle_count_ << "\t" << active_fault_ << "\t" << "abort signal detected" << "\t" << a->name_ << std::endl;
      }
      // After a signal is asserted, wait for 'delay' cycles before signalling
      // the stop request
      if (a->delay_count > 0) {
        a->delay_count--;
      } else {
        log_ << cycle_count_ << "\t" << active_fault_ << "\t" << "abort signal dealy expired" << "\t" << a->name_ << std::endl;
        return true;
      }
    }
  }

  // Compare current values against comparison list
  for (auto m : value_compare_list_) {
    std::string log;
    if (m(log)) {
      log_ << cycle_count_ << "\t" << active_fault_ << "\t" << "data match" << "\t" << log << std::endl;
    }
  }
  return false;
}

void FaultInjection::AddValueComparator(std::function<bool (std::string &)>& fs) {
  value_compare_list_.push_back(fs);
}

std::ostream & operator<<(std::ostream& os, const struct Fault& f) {
  os << f.temporal << "," << f.spatial;
  return os;
}

std::ostream & operator<<(std::ostream& os, const FaultInjection& f) {
  os << f.log_.str();
  return os;
}
