#ifndef CONTRIB_SHARED_INFO_H
#define CONTRIB_SHARED_INFO_H
#include <unordered_set>

struct SubproblemData {
  IloNumArray obj_coeffs;
  IloNumArray constraint_lb, constraint_ub;
  IloNumArray2 constraint_coeffs;
};

struct ProblemInfo {
  bool is_minimization{true};
  bool is_maximization{false};
  bool is_determinstic{false};
  bool is_stochastic = !is_determinstic;
  bool is_obj_coeff_stochastic{false};
  bool is_RHS_stochastic{false};
  bool is_constraint_coeff_uncertain{false};
  bool is_fixed_recourse{false};
};

// to keep the int sols that MIP SPs gen
struct CopiedVarsValPool {
  // uint32_t frequency{0};
  // uint32_t sp_id;
  // uint32_t sol_id;
  std::vector<int> solution;
  // I dont want to skip repetion
  inline bool operator==(const CopiedVarsValPool &other) const {
    assert(solution.size() == other.solution.size());
    return false;  // solution == other.solution;
  }
  inline bool operator<(const CopiedVarsValPool &other) const {
    return true;  // solution == other.solution ? false : true;
  }
};
// struct CopiedVarsValPoolHasher {
//   std::size_t operator()(const CopiedVarsValPool &other) const {
//     return other.sp_id ^ other.sol_id;
//   }
// };

struct SharedInfo {
  uint64_t num_subproblems{0};
  uint64_t num_master_variables{0};
  uint64_t num_recourse_variables{0};
  uint64_t num_master_constraints{0};

  IloNumArray master_variables_lb;
  IloNumArray master_variables_ub;
  IloNumArray master_variables_value;
  std::vector<double> master_variables_value_lp;
  IloNumArray master_vars_reduced_cost;
  IloNumArray recourse_variables_value;

  IloNumArray subproblem_weight;
  IloNumArray master_variable_obj_coeff;
  std::vector<SubproblemData> subproblem_data;

  ProblemInfo problem_info;

  std::set<CopiedVarsValPool> copied_vars_val_pool;

  std::set<uint64_t> retained_subproblem_ids;

  std::vector<double> subproblem_objective_value;
  std::vector<int> subproblem_status;  // true:opt, false:inf
  std::vector<IloNumArray> dual_values;
  std::vector<IloNumArray> copied_variables_value;
};

#endif
