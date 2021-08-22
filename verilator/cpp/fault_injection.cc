#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include "fault_injection.h"

FaultInjection::FaultInjection(unsigned int fi_signal_len, int temporal_start, int temporal_duration, bool mode_linear, unsigned long int iteration_count) : num_fi_signals(fi_signal_len) {
  injected_ = false;
  cycle_count_ = 0;
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
}

FaultInjection::FaultInjection(unsigned int fi_signal_len, int fault_temporal, int fault_spatial) : num_fi_signals(fi_signal_len) {
  injected_ = false;
  cycle_count_ = 0;
  active_fault_ = Fault {fault_temporal, fault_spatial};
}

std::pair<int, int> FaultInjection::GetFaultSpace() {
  return std::make_pair(active_fault_.temporal, active_fault_.spatial);
}

bool FaultInjection::Injected() {
  return injected_;
}

void FaultInjection::DumpConfig(std::ofstream &olog) {
      olog << active_fault_ << std::endl;
}

void FaultInjection::AddAbortWatch(CData *signal, unsigned int delay, bool positive_polarity) {
  abort_watch_list_.push_back(AbortInfo{signal, positive_polarity, delay, false});
}

bool FaultInjection::StopRequested() {
  // Check for an abort signal
  for (auto a = abort_watch_list_.begin(); a != abort_watch_list_.end(); ++a) {
    // Store a signal assertion
    if (*a->signal == a->positive_polarity) {
      a->asserted = true;
    }
    if (a->asserted) {
      // After a signal is asserted, wait for 'delay' cycles before signalling
      // the stop request
      if (a->delay > 0) {
        a->delay--;
      } else {
        return true;
      }
    }
  }

  // Compare current values against comparison list
  for (auto m : value_compare_list_) {
    if (m()) {
      return true;
    }
  }
  return false;
}

void FaultInjection::AddValueComparator(std::function<bool ()>& fs) {
  value_compare_list_.push_back(fs);
}

std::ostream & operator<<(std::ostream& os, const struct Fault& f) {
  os << f.temporal << "," << f.spatial;
  return os;
}

std::ostream & operator<<(std::ostream& os, const FaultInjection& f) {
  os << f.active_fault_;
  return os;
}
