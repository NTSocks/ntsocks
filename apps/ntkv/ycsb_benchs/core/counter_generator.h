#ifndef YCSB_C_COUNTER_GENERATOR_H_
#define YCSB_C_COUNTER_GENERATOR_H_

#include "generator.h"

#include <cstdint>
#include <atomic>

namespace ycsbc {

class CounterGenerator : public Generator<uint64_t> {
 public:
  CounterGenerator(uint64_t start) : counter_(start) { }
  uint64_t Next() { return counter_.fetch_add(1); }
  uint64_t Last() { return counter_.load() - 1; }
  void Set(uint64_t start) { counter_.store(start); }
 private:
  std::atomic<uint64_t> counter_;
};

} // ycsbc

#endif // YCSB_C_COUNTER_GENERATOR_H_
