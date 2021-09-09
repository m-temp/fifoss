#ifndef DATA_MONITOR_H_
#define DATA_MONITOR_H_

#include <cstddef>

// TODO: add option to compare arrays

template<typename T>
class DataMonitor {
  public:
    DataMonitor(T *signal, T compare_values[], size_t compare_length);
    bool Compare(void);
  private:
    T *signal_;
    T *compare_values_;
    size_t compare_length_;
};

template<typename T>
bool DataMonitor<T>::Compare(void) {
  for (size_t i=0; i < compare_length_; ++i) {
    if (compare_values_[i] == *signal_) {
      return true;
    }
  }
  return false;
}

template<typename T>
DataMonitor<T>::DataMonitor(T *signal, T compare_values[], size_t compare_length) {
  signal_ = signal;
  compare_values_ = compare_values;
  compare_length_ = compare_length;
}

#endif // DATA_MONITOR_H_
