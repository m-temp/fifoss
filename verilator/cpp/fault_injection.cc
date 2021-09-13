#include "fault_injection.h"

#include <getopt.h>

#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <utility>

FaultInjection::FaultInjection(unsigned int fi_signal_len)
    : num_fi_signals(fi_signal_len),
      injected_(false),
      injection_duration_(1),
      cycle_count_(0),
      num_iterations_(1),
      sequential_(false),
      inject_specific_(false) {
  // Set default values
  active_fault_ = Fault{1, 1};
  temporal_limit_ = Temporal{1, 1};
}

void FaultInjection::SetModeRange(unsigned int temporal_start,
                                  unsigned int temporal_duration,
                                  bool mode_sequential,
                                  unsigned long int iteration_count) {
  temporal_limit_ = Temporal{temporal_start, temporal_duration};
  sequential_ = mode_sequential;
  SetFaultRange(iteration_count);
}

void FaultInjection::SetModePrecise(unsigned int fault_temporal,
                                    unsigned int fault_spatial) {
  active_fault_ = Fault{fault_temporal, fault_spatial};
  log_ << "Fault injection configured with:\nfault signal width: "
       << num_fi_signals << "\nfault cycle: " << active_fault_.temporal
       << "\nfault signal number: " << active_fault_.spatial << std::endl;
}

void FaultInjection::UpdateSpace(unsigned long int iteration_count) {
  log_.clear();
  log_.str("");
  injected_ = false;
  SetFaultRange(iteration_count);
}

void FaultInjection::SetFaultRange(unsigned long int iteration_count) {
  // Two different ways to set the fault for a specific run.
  if (sequential_) {
    // Sequential mode needs the current iteration number and will then iterate
    // over the space. Low frequency for clock and high frequency for position.
    active_fault_.temporal =
        iteration_count / num_fi_signals + temporal_limit_.start;
    active_fault_.spatial = iteration_count % num_fi_signals;
  } else {
    // Choose values randomly
    // TODO: Allow to set a manual seed or at some other randomness.
    active_fault_.temporal =
        rand() % temporal_limit_.start + temporal_limit_.duration;
    active_fault_.spatial = rand() % (num_fi_signals + 1);
  }
  log_ << "Fault injection configured with:\n\tfault cycle ["
       << temporal_limit_.start << ":" << temporal_limit_.duration << "]:\t"
       << active_fault_.temporal
       << "\n\tfault signal number [0:" << num_fi_signals - 1 << "]:\t"
       << active_fault_.spatial << std::endl;
}

std::pair<int, int> ExtractPairValue(std::string str) {
  std::istringstream iss;
  int first, second;
  char separator;
  iss.str(str);
  // Number; single character as separator; number
  iss >> first >> separator >> second;
  return std::make_pair(first, second);
}

bool FaultInjection::ParseCommandArgs(int argc, char **argv, bool &exit_app) {
  // TODO: option to read already tested combination, continue fault injection
  const struct option long_options[] = {
      {"iterations", required_argument, nullptr, 'n'},
      {"sequential", no_argument, nullptr, 's'},
      {"inject", required_argument, nullptr, 'i'},
      {"temporal-limits", required_argument, nullptr, 'z'},
      {"help", no_argument, nullptr, 'h'},
      {nullptr, no_argument, nullptr, 0}};
  optind = 1;
  std::pair<int, int> inject_space;
  std::pair<int, int> temporal_limit;

  while (1) {
    int c = getopt_long(argc, argv, ":n:si:z:h", long_options, nullptr);
    if (c == -1) {
      break;
    }
    // Disable error reporting by getopt
    opterr = 0;

    switch (c) {
      case 0:
        break;
      case 'h':
        std::cout
            << "Fault injection analysis options:\n"
               "=================================\n\n"
               "-n|--iterations=N\n  Number of simulation iterations\n\n"
               "-s|--sequential\n  Consecutively cycle through the fault space "
               "(spatially with high frequency) instead of randomly\n\n"
               "-i|--inject=t,p\n  Set cycle and position for fault "
               "injection\n\n"
               "-z|--temporal-limits=t0,td\n  Restrict temporal space\n"
               "  Start time,Duration\n\n"
            << std::endl;
        exit_app = true;
        break;
      case 'n':
        num_iterations_ = std::stoul(optarg);
        break;
      case 's':
        sequential_ = true;
        break;
      case 'i':
        // Parse data from "12,34"
        inject_space = ExtractPairValue(optarg);
        active_fault_ = Fault{static_cast<unsigned int>(inject_space.first),
                              static_cast<unsigned int>(inject_space.second)};
        inject_specific_ = true;
        break;
      case 'z':
        temporal_limit = ExtractPairValue(optarg);
        temporal_limit_ =
            Temporal{static_cast<unsigned int>(temporal_limit.first),
                     static_cast<unsigned int>(temporal_limit.second)};
        break;
      case ':':  // missing argument
        std::cerr << "ERROR: Missing argument." << std::endl << std::endl;
        exit_app = true;
        return false;
      case '?':
      default:;
        // Ignore unrecognized options since they might be consumed by
        // Verilator's built-in parsing below.
    }
  }
  if (!inject_specific_) {
    SetFaultRange();
  }
  return true;
}

unsigned int FaultInjection::IterationLength() {
  if (inject_specific_) {
    // For now only one specific testing at a time
    return 1;
  }
  return num_iterations_;
}

struct Fault FaultInjection::GetFaultSpace() {
  return active_fault_;
}

bool FaultInjection::Injected() { return injected_; }

void FaultInjection::DumpConfig(std::ofstream &olog) {
  olog << active_fault_ << std::endl;
}

void FaultInjection::AddAbortWatch(const char *name, CData *signal,
                                   unsigned int delay, bool positive_polarity) {
  abort_watch_list_.push_back(
      AbortInfo{name, signal, positive_polarity, delay, delay, false});
}

bool FaultInjection::StopRequested() {
  // Only check after fault is inserted
  if (!injected_) {
    return false;
  }
  // Check for an abort signal
  for (auto a = abort_watch_list_.begin(); a != abort_watch_list_.end(); ++a) {
    // Store a signal assertion
    if (*a->signal == a->positive_polarity) {
      a->asserted = true;
    }
    if (a->asserted) {
      if (a->delay == a->delay_count) {
        log_ << cycle_count_ << "\t" << active_fault_ << "\t"
             << "abort signal detected"
             << "\t" << a->name_ << std::endl;
      }
      // After a signal is asserted, wait for 'delay' cycles before signalling
      // the stop request
      if (a->delay_count > 0) {
        a->delay_count--;
      } else {
        log_ << cycle_count_ << "\t" << active_fault_ << "\t"
             << "abort signal dealy expired"
             << "\t" << a->name_ << std::endl;
        return true;
      }
    }
  }

  // Compare current values against comparison list
  for (auto m : value_compare_list_) {
    std::string log;
    if (m(log)) {
      log_ << cycle_count_ << "\t" << active_fault_ << "\t"
           << "data match"
           << "\t" << log << std::endl;
    }
  }
  return false;
}

void FaultInjection::AddValueComparator(
    std::function<bool(std::string &)> &fs) {
  value_compare_list_.push_back(fs);
}

std::ostream &operator<<(std::ostream &os, const struct Fault &f) {
  os << f.temporal << "," << f.spatial;
  return os;
}

std::ostream &operator<<(std::ostream &os, const FaultInjection &f) {
  os << f.log_.str();
  return os;
}
