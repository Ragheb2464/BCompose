#ifndef UTIL_FUNCTIONS_H
#define UTIL_FUNCTIONS_H

struct Hasher {
  inline uint64_t operator()(const std::pair<uint64_t, uint64_t> &pair) const {
    return pair.first ^ pair.second;
  }
};

#endif
