#ifndef DATA_MONITOR_H_
#define DATA_MONITOR_H_

#include <cstddef>
#include <cstdio>
#include <string>

// TODO: add option to compare arrays

template <typename T>
class DataMonitor {
 public:
  DataMonitor(const char *name, T *signal, T compare_values[],
              size_t compare_length);
  bool Compare(std::string &log);

 private:
  const std::string name_;
  T *signal_;
  T *compare_values_;
  size_t compare_length_;
};

template <typename T>
bool DataMonitor<T>::Compare(std::string &log) {
  char l[16];
  for (size_t i = 0; i < compare_length_; ++i) {
    if (compare_values_[i] == *signal_) {
      // Using snprintf instead of ostringstream because only with printf like
      // formatting the values of T are converted to a hex representation
      // without knowing the basic type (e.g. CData vs unsigned short).
      log = name_;
      std::snprintf(l, sizeof(l), " 0x%x", compare_values_[i]);
      log += l;
      return true;
    }
  }
  return false;
}

template <typename T>
DataMonitor<T>::DataMonitor(const char *name, T *signal, T compare_values[],
                            size_t compare_length)
    : name_(name) {
  signal_ = signal;
  compare_values_ = compare_values;
  compare_length_ = compare_length;
}

#endif  // DATA_MONITOR_H_
