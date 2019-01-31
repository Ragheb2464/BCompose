#ifndef EXTERNALS_UTIL_RAND_H
#define EXTERNALS_UTIL_RAND_H

#include <random>
std::mt19937 random_gen(42);

uint64_t RandInt(const uint64_t lb, const uint64_t ub) {
  std::uniform_int_distribution<uint64_t> uniform(lb, ub);
  return uniform(random_gen);
}
double Rand01() {
  std::uniform_real_distribution<double> uniform(0, 1);
  return uniform(random_gen);
}
double RandRealBetween(const uint64_t lb, const uint64_t ub) {
  std::uniform_real_distribution<double> uniform(lb, ub);
  return uniform(random_gen);
}
#endif
