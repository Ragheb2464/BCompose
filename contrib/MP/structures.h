#ifndef CONTRIB_MP_STRUCTURES_H
#define CONTRIB_MP_STRUCTURES_H

struct MasterSolverInfo {
  double LB{-IloInfinity};
  double global_UB{IloInfinity};
  double lp_phase_UB{IloInfinity};
  double lp_phase_LB{-IloInfinity};
  double LB_after_lifter{-IloInfinity};
  double current_LB;
  double current_UB;
  uint32_t lp_iteration{0};
  uint32_t iteration{0};
  uint32_t num_eq_iterations{0};
  uint64_t num_nodes{0};
  double init_time{0};
  double lp_time{0};
  uint64_t lp_num_feas{0};
  uint64_t lp_num_opt{0};
  uint64_t num_feas{0};
  uint64_t num_opt{0};
  std::chrono::steady_clock::time_point start_time;
  float duration;
  bool gen_user_cuts{true};

  std::vector<double> sp_weights_to_create_art_sp;
};

struct MasterModel {
  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective objective;
  IloRangeArray constraints;
  IloRangeArray opt_cuts;
  IloRangeArray feas_cuts;
  IloNumVarArray variables;
  IloNumVarArray master_variables;
  IloNumVarArray recourse_variables;
};

#endif
