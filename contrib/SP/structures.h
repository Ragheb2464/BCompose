#ifndef CONTRIB_SP_STRUCTURES_H
#define CONTRIB_SP_STRUCTURES_H

struct SubproblemSolverInfo {
  std::chrono::steady_clock::time_point start_time;
  uint64_t iteration{0};
  uint64_t num_solved_sp_so_far{0};
};

struct SubproblemModel {
  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective objective;
  IloObjective regular_objective;
  IloObjective slack_objective;
  IloRangeArray constraints;
  IloRangeArray NAC_constraints;
  IloNumVarArray variables;
  IloNumVarArray copied_variables;
  IloNumVarArray slack_variables;
  IloNumArray slacks_lb, slacks_ub;
};

#endif
