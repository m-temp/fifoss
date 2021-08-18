#include <fstream>
#include <iostream>
#include <cstdlib>
#include <utility>
#include "fault_injection.h"

FaultInjection::FaultInjection(unsigned int fi_signal_len, struct temporal temporal_limit, bool mode_linear, unsigned long int iteration_count) : num_fi_signals(fi_signal_len) {
  injected = false;
  cycle_count = 0;

  // Two different ways to set the fault for a specific run.
  if (mode_linear) {
    // Linear mode need the current iteration number and will then iterate over the space.
    // Low frequency for clock and high frequency for position.
    active_fault.temporal = iteration_count / num_fi_signals + temporal_limit.start;
    active_fault.spatial = iteration_count % num_fi_signals;
  } else {
    // Choose values randomly
    // TODO: Allow to set a manual seed or at some other randomness.
    active_fault.temporal = rand() % temporal_limit.start + temporal_limit.duration;
    active_fault.spatial = rand() % (num_fi_signals + 1);
  }
}

FaultInjection::FaultInjection(unsigned int fi_signal_len, int fault_temporal, int fault_spatial) : num_fi_signals(fi_signal_len) {
  injected = false;
  cycle_count = 0;
  active_fault = fault {fault_temporal, fault_spatial};
}

std::pair<int, int> FaultInjection::GetFaultSpace() {
  return std::make_pair(active_fault.temporal, active_fault.spatial);
}

bool FaultInjection::Injected() {
  return injected;
}

void FaultInjection::SaveToLog(std::ofstream &olog) {
      olog << active_fault.temporal << "," << active_fault.spatial << std::endl;
}

void FaultInjection::AddAbortWatch(CData *signal, unsigned int delay, bool positive_polarity) {
  abort_watch.push_back(abortWatch{signal, positive_polarity, delay, false});
}

bool FaultInjection::StopRequested() {
  // Check for an abort signal
  for (auto a = abort_watch.begin(); a != abort_watch.end(); ++a) {
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
  return false;
}