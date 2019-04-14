#ifndef COTRIB_MP_CALLBACKS_H
#define COTRIB_MP_CALLBACKS_H
/** @file callbacks.h
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/7/2018
@brief    This part manages the B&B tree with the callbacks.
*/
#include "../SP/subproblem.h"
#include "../control/solver_settings.h"
#include "../heuristic/heuristic.h"
#include "../shared_info/structures.h"
#include "structures.h"

class BendersCustomCutCallback : public IloCplex::Callback::Function {
public:
  BendersCustomCutCallback(const std::shared_ptr<spdlog::logger> &_console,
                           const SharedInfo &_shared_info,
                           const IloNumVarArray &_master_variables,
                           const IloNumVarArray &_recourse_variables,
                           const MasterSolverInfo &_solver_info_,
                           const std::shared_ptr<Subproblem> &_SP)
      : console_(_console), shared_info_(_shared_info),
        master_variables_(_master_variables),
        recourse_variables_(_recourse_variables), solver_info_(_solver_info_),
        SP_(_SP) {}

  void invoke(const IloCplex::Callback::Context &context);
  void AddLazyCuts(const IloCplex::Callback::Context &context);
  MasterSolverInfo GetSolverInfo() { return solver_info_; }

private:
  std::shared_ptr<spdlog::logger> console_;
  SharedInfo shared_info_;
  IloNumVarArray master_variables_;
  IloNumVarArray recourse_variables_;
  MasterSolverInfo solver_info_;
  std::shared_ptr<Subproblem> SP_;
  /* Empty constructor is forbidden. */
  BendersCustomCutCallback() {}
  /* Copy constructor is forbidden. */
  BendersCustomCutCallback(const BendersCustomCutCallback &tocopy);
};

void BendersCustomCutCallback::invoke(
    const IloCplex::Callback::Context &context) {
  const uint32_t threadNo =
      context.getIntInfo(IloCplex::Callback::Context::Info::ThreadId);
  assert(threadNo == 0);

  switch (context.getId()) {
  case IloCplex::Callback::Context::Id::Candidate:
    if (!context.isCandidatePoint()) { // The model is always bounded
      throw IloCplex::Exception(-1, "Unbounded solution");
    }
    AddLazyCuts(context);
    break;
  case IloCplex::Callback::Context::Id::Relaxation:
    throw IloCplex::Exception(-1, "Does not support contextID==Relaxation");
    break;
  default:
    throw IloCplex::Exception(-1, "Unexpected contextID");
  }
}

void BendersCustomCutCallback::AddLazyCuts(
    const IloCplex::Callback::Context &context) {
  assert(context.getId() == IloCplex::Callback::Context::Id::Candidate);

  IloEnv env = context.getEnv();
  context.getCandidatePoint(recourse_variables_,
                            shared_info_.recourse_variables_value);
  context.getCandidatePoint(master_variables_,
                            shared_info_.master_variables_value);

  SP_->GenBendersCuts(console_, shared_info_);
  bool is_sol_infes = false;
  double total_violation = 0;
  uint64_t num_violated_cuts = 0;
  IloRangeArray violated_cuts(env);
  for (uint64_t sp_id = 0; sp_id < shared_info_.num_subproblems; ++sp_id) {
    if (shared_info_.retained_subproblem_ids.count(sp_id)) {
      continue;
    }
    IloExpr expr(env);
    double violation_sum = 0;
    expr += shared_info_.subproblem_objective_value[sp_id];
    violation_sum += shared_info_.subproblem_objective_value[sp_id];
    assert(shared_info_.dual_values[sp_id].getSize() ==
           master_variables_.getSize());
    expr -= IloScalProd(shared_info_.dual_values[sp_id],
                        shared_info_.master_variables_value);
    expr += IloScalProd(shared_info_.dual_values[sp_id], master_variables_);

    if (shared_info_.subproblem_status[sp_id]) {
      ++solver_info_.num_opt;
      expr -= recourse_variables_[sp_id];
      violation_sum -= shared_info_.recourse_variables_value[sp_id];
    } else {
      is_sol_infes = true;
      ++solver_info_.num_feas;
      // assert(shared_info_.subproblem_objective_value[sp_id] > 0);
    }
    total_violation += violation_sum;
    if (violation_sum > 1e-6) {
      ++num_violated_cuts;
      violated_cuts.add(expr <= 0);
    }
    expr.end();
  }
  if (total_violation > 1e-6) {
    context.rejectCandidate(violated_cuts);
  }
  ++solver_info_.iteration;
  {
    std::chrono::duration<float> diff =
        std::chrono::steady_clock::now() - solver_info_.start_time;
    solver_info_.duration = diff.count();
    // console_->info("**Added " + std::to_string(num_violated_cuts) +
    //                " lazy cuts.");
    console_->info("   UB=" + ValToStr(context.getIncumbentObjective()) +
                   " Candidate=" + ValToStr(context.getCandidateObjective()) +
                   " #Cuts=" + std::to_string(num_violated_cuts) +
                   " Time=" + ValToStr(solver_info_.duration) +
                   " Iteration=" + std::to_string(solver_info_.iteration));
  }
}

// ILOLAZYCONSTRAINTCALLBACK6(BendersLazyCallback,
//                            const std::shared_ptr<spdlog::logger>, console,
//                            SharedInfo &, shared_info,
//                            std::unique_ptr<Subproblem> &, SP, IloNumVarArray,
//                            master_variables, IloNumVarArray,
//                            recourse_variables, MasterSolverInfo &,
//                            solver_info_) {
//   IloEnv env = getEnv();
//   getValues(shared_info.recourse_variables_value, recourse_variables);
//   getValues(shared_info.master_variables_value, master_variables);
//   SP->GenBendersCuts(console, shared_info);
//   bool is_sol_infes = false;
//   for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
//     if (shared_info.retained_subproblem_ids.count(sp_id)) {
//       continue;
//     }
//     IloExpr expr(env);
//     expr += shared_info.subproblem_objective_value[sp_id];
//     assert(shared_info.dual_values[sp_id].getSize() ==
//            master_variables.getSize());
//     expr -= IloScalProd(shared_info.dual_values[sp_id],
//                         shared_info.master_variables_value);
//     expr += IloScalProd(shared_info.dual_values[sp_id], master_variables);
//
//     if (shared_info.subproblem_status[sp_id]) {
//       solver_info_.num_opt++;
//       expr -= recourse_variables[sp_id];
//       add(expr <= 0).end();
//     } else {
//       is_sol_infes = true;
//       solver_info_.num_feas++;
//       assert(shared_info.subproblem_objective_value[sp_id] > -1e-7);
//       add(expr <= 0).end();
//     }
//     expr.end();
//   }
//   {
//     if (ProblemSpecificSettings::AdvancedCuts::gen_combinatorial_cuts &&
//         is_sol_infes) {
//       IloExpr expr(env);
//       for (size_t var_id = 0;
//            var_id < shared_info.master_variables_value.getSize(); var_id++) {
//         if (shared_info.master_variables_value[var_id]) {
//           if (!ProblemSpecificSettings::AdvancedCuts::
//                   gen_strengthened_combinatorial_cuts) {
//             expr += 1 - master_variables[var_id];
//           }
//           continue;
//         }
//         expr += master_variables[var_id];
//       }
//       // console->info("Adding BCB ");
//       add(expr >= 1);
//       expr.end();
//     }
//   }
//   solver_info_.iteration++;
//   {
//     std::chrono::duration<float> diff =
//         std::chrono::steady_clock::now() - solver_info_.start_time;
//     solver_info_.duration = diff.count();
//     console->info("   LB=" + ValToStr(getBestObjValue()) +
//                   " UB=" + ValToStr(getIncumbentObjValue()) +
//                   " NumNodes=" + std::to_string(getNnodes()) +
//                   " Gap%=" + ValToStr(100 * getMIPRelativeGap()) +
//                   " Time=" + ValToStr(solver_info_.duration) +
//                   " Iteration=" + std::to_string(solver_info_.iteration));
//   }
//
//   return;
// }  // END BendersLazyCallback
//
// ILOUSERCUTCALLBACK6(BendersUserCallback, const
// std::shared_ptr<spdlog::logger>,
//                     console, SharedInfo &, shared_info,
//                     std::unique_ptr<Subproblem> &, SP, IloNumVarArray,
//                     master_variables, IloNumVarArray, recourse_variables,
//                     MasterSolverInfo &, solver_info_) {
//   const auto nodeId = getNnodes();
//   if (solver_info_.gen_user_cuts && Settings::CutGeneration::use_LR_cuts &&
//       isAfterCutLoop() && nodeId == 0) {
//     console->info(" -Booster still looking...");
//     IloEnv env = getEnv();
//     getValues(shared_info.recourse_variables_value, recourse_variables);
//     getValues(shared_info.master_variables_value, master_variables);
//     // std::cout << shared_info.master_variables_value << std::endl;
//     SP->GenBendersCuts(console, shared_info);
//     SP->GenAdvancedCuts(console, shared_info);
//     for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
//       if (shared_info.retained_subproblem_ids.count(sp_id)) {
//         continue;
//       }
//       double fixed_part;
//       const bool status = shared_info.subproblem_status[sp_id];
//       if (status) {  // we have lifted only opt cuts
//         solver_info_.num_opt++;
//         fixed_part = shared_info.subproblem_objective_value[sp_id] -
//                      IloScalProd(shared_info.dual_values[sp_id],
//                                  shared_info.copied_variables_value[sp_id]);
//       } else {  // feas cuts
//         solver_info_.num_feas++;
//         fixed_part = shared_info.subproblem_objective_value[sp_id];
//         assert(fixed_part > 0);
//         fixed_part -= IloScalProd(shared_info.dual_values[sp_id],
//                                   shared_info.master_variables_value);
//       }
//       IloExpr expr(env);
//       expr += fixed_part;
//       expr += IloScalProd(shared_info.dual_values[sp_id], master_variables);
//       if (status) {
//         expr -= recourse_variables[sp_id];
//         add(expr <= 0).end();
//       } else {
//         add(expr <= 0).end();
//       }
//       expr.end();
//     }
//
//     if (100 * (getBestObjValue() - solver_info_.LB) / getBestObjValue() <=
//         0.1) {
//       solver_info_.gen_user_cuts = false;
//       SP->DeleteLRSubproblems();
//       console->info(
//           " +Booster successfully terminated with " +
//           ValToStr(100 * (getBestObjValue() - solver_info_.lp_phase_UB) /
//                    (1e-7 + solver_info_.lp_phase_UB)) +
//           "% lift.");
//     }
//     solver_info_.LB = getBestObjValue();
//     solver_info_.LB_after_lifter = solver_info_.LB;
//     solver_info_.iteration++;
//     std::chrono::duration<float> diff =
//         std::chrono::steady_clock::now() - solver_info_.start_time;
//     solver_info_.duration = diff.count();
//     console->info("   LB=" + ValToStr(getBestObjValue()) +
//                   " UB=" + ValToStr(getIncumbentObjValue()) +
//                   " NumNodes=" + std::to_string(getNnodes()) +
//                   " Gap%=" + ValToStr(100 * getMIPRelativeGap()) +
//                   " Time=" + ValToStr(solver_info_.duration) +
//                   " Iteration=" + std::to_string(solver_info_.iteration));
//   } else if (nodeId > 0 && nodeId > Settings::Heuristic::start_node &&
//              nodeId <= Settings::Heuristic::frequency +
//                            Settings::Heuristic::start_node &&
//              Settings::Heuristic::run_as_heuristic) {
//     console->info("   Heuristic searching");
//     uint64_t num_fixed_lb = 0;
//     uint64_t num_fixed_ub = 0;
//     double tolerance = Settings::Heuristic::aggressiveness;
//     getValues(shared_info.master_variables_value, master_variables);
//     for (int var_id = 0; var_id <
//     shared_info.master_variables_value.getSize();
//          ++var_id) {
//       if (master_variables[var_id].getLB() ==
//           master_variables[var_id].getUB()) {
//         continue;
//       }
//       if (std::fabs(shared_info.master_variables_value[var_id] -
//                     master_variables[var_id].getLB()) < tolerance) {
//         addLocal(master_variables[var_id] <=
//         master_variables[var_id].getLB());
//         ++num_fixed_lb;
//       } else if (std::fabs(shared_info.master_variables_value[var_id] -
//                            master_variables[var_id].getUB()) < tolerance) {
//         addLocal(master_variables[var_id] >=
//         master_variables[var_id].getUB());
//         ++num_fixed_ub;
//       }
//     }
//
//     if (num_fixed_lb + num_fixed_ub) {
//       console->info("   Heuristics (" + std::to_string(nodeId) + "," +
//                     std::to_string(num_fixed_lb + num_fixed_ub) + ")...");
//     }
//     abortCutLoop();
//   } else if (!solver_info_.gen_user_cuts && nodeId == 0 &&
//              Settings::Heuristic::run_lagrang_fixer) {
//     console->info("  Heuristic fixer searching...");
//     Heuristic heur;
//     const bool has_pool_sol = heur.SetVarValFreq(console, shared_info);
//     uint64_t num_fixed = 0;
//     if (has_pool_sol) {
//       heur.SetNumSolsInPool();
//       for (size_t var_id = 0; var_id < master_variables.getSize(); var_id++)
//       {
//         const auto pair_bool_val = heur.GenVarStatus(var_id);
//         if (pair_bool_val.first) {
//           const uint32_t val = pair_bool_val.second;
//           assert(val >= master_variables[var_id].getLB() &&
//                  val <= master_variables[var_id].getUB());
//           add(master_variables[var_id] == val);
//           // master_model_.master_variables[var_id].setUB(val);
//           ++num_fixed;
//         }
//       }
//     }
//     console->info("   " + std::to_string(num_fixed) +
//                   " variables got permenantly fixed out of " +
//                   std::to_string(master_variables.getSize()));
//     abortCutLoop();
//   }
//   return;
// }

#endif
