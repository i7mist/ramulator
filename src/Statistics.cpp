#if !defined(RAMULATOR)
#define INTEGRATED_WITH_GEM5
#endif

#ifndef INTEGRATED_WITH_GEM5
#include "Statistics.h"

namespace Stats {
  Temp::Temp(const ramulator::ScalarStat &s):op(), leaf(s.get_stat()) {}
  Temp::Temp(const ramulator::AverageStat &s):op(), leaf(s.get_stat()) {}
  Temp::Temp(const ramulator::VectorStat &s):op(), leaf(s.get_stat()) {}
  Temp::Temp(const ramulator::AverageVectorStat &s):op(), leaf(s.get_stat()) {}

}  // namespace Stats

#endif
