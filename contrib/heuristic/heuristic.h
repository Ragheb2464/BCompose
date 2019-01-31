#ifndef CONTRIB_HEURISTIC_CLASS_HEURISTIC_H
#define CONTRIB_HEURISTIC_CLASS_HEURISTIC_H

#include "../../externals/spdlog/spdlog.h"
#include "../../externals/util/rand.h"
#include "../shared_info/structures.h"

class Heuristic {
public:
  Heuristic(){};
  ~Heuristic(){};
  /*This func counts how many times a var has taken a specific value in the
   * solutions extracted from the MIP SPs*/
  bool SetVarValFreq(const std::shared_ptr<spdlog::logger> console,
                     const SharedInfo &shared_info) {
    // for each master var, we map to all its values it has taken and its
    // frequency
    for (const auto &it : shared_info.copied_vars_val_pool) {
      const auto &solution = it.solution;
      for (uint64_t var_id = 0; var_id < solution.size(); ++var_id) {
        const auto val = solution[var_id];
        auto &res_map = sol_vals_freq[var_id];
        const auto &res = res_map.emplace(val, 0);
        if (!res.second) {
          res.first->second++;
        }
      }
    }
    return sol_vals_freq.size() ? true : false;
  }
  void PrintVarValsFreq(const std::shared_ptr<spdlog::logger> console,
                        const uint64_t var_id) {
    // print stuff
    assert(sol_vals_freq.count(var_id));
    for (const auto &stat : sol_vals_freq.at(var_id)) {
      console->info("y_" + std::to_string(var_id) + " has taken " +
                    std::to_string(stat.first) + " value " +
                    std::to_string(stat.second) + " times.");
    }
  }

  uint64_t GetValFreq(const uint64_t var_id, const uint64_t val) {
    assert(sol_vals_freq.count(var_id));
    return sol_vals_freq.at(var_id).count(val)
               ? sol_vals_freq.at(var_id).at(val)
               : 0;
  }
  void SetNumSolsInPool() noexcept(true) {
    num_sols_in_pool = 0;
    for (const auto it : sol_vals_freq.at(0)) {
      num_sols_in_pool += it.second;
    }
  }
  uint64_t GenRandVarVal(const uint64_t var_id) {
    assert(sol_vals_freq.count(var_id));
    assert(num_sols_in_pool > 0);
    const double rand_num = Rand01();
    double accum_weight = 0;
    for (const auto it : sol_vals_freq.at(var_id)) {
      accum_weight += it.second / (num_sols_in_pool + 0.0);
      if (rand_num <= accum_weight) {
        return it.first;
      }
    }
    std::cout << rand_num << " " << accum_weight << std::endl;
    assert(false);
  }
  std::pair<bool, uint64_t> GenVarStatus(const uint64_t var_id) {
    assert(sol_vals_freq.count(var_id));
    assert(num_sols_in_pool > 0);
    int max_freq = 0;
    std::pair<bool, uint64_t> bool_val = std::make_pair(false, 0);
    for (const auto it : sol_vals_freq.at(var_id)) {
      if (it.second >= Settings::Heuristic::tolerance * num_sols_in_pool &&
          it.second > max_freq) {
        bool_val.first = true;
        bool_val.second = it.first;
        max_freq = it.second;
      }

      // printf("%d - %d - %d\n", var_id, it.first, it.second);
    }
    return std::move(bool_val);
  }

private:
  std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint64_t>>
      sol_vals_freq; // var_id=>val=>freq
  uint64_t num_sols_in_pool;
};
#endif
