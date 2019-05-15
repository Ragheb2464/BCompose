#ifndef COTRIB_MP_CALLBACKS_H
#define COTRIB_MP_CALLBACKS_H
/** @file callbacks.h
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/7/2018
@brief    This part manages the B&B tree with the callbacks.
*/
#include <mutex> // std::mutex

#include "../../solver_settings.h"
#include "../SP/subproblem.h"
#include "../heuristic/heuristic.h"
#include "../shared_info/structures.h"
#include "structures.h"

class BendersCustomCutCallback : public IloCplex::Callback::Function {
public:
  explicit BendersCustomCutCallback(
      const std::shared_ptr<spdlog::logger> _console,
      const SharedInfo &_shared_info, const IloNumVarArray &_master_variables,
      const IloNumVarArray &_recourse_variables,
      const MasterSolverInfo &_solver_info_,
      const std::shared_ptr<Subproblem> _SP)
      : console_(_console), shared_info_(_shared_info),
        master_variables_(_master_variables),
        recourse_variables_(_recourse_variables), solver_info_(_solver_info_),
        SP_(_SP) {
    solver_info_.previous_root_obj =
        solver_info_.lp_phase_LB * (100 - 2 * _min_lift_percentage);
    solver_info_.LB_after_lifter = solver_info_.lp_phase_LB;
  }

  BendersCustomCutCallback(const BendersCustomCutCallback &) = delete;
  ~BendersCustomCutCallback() {
    master_variables_.end();
    recourse_variables_.end();
  }

  inline void invoke(const IloCplex::Callback::Context &context);
  inline void AddLazyCuts(const IloCplex::Callback::Context &context);
  inline void AddLRCuts(const IloCplex::Callback::Context &context);
  inline MasterSolverInfo &GetSolverInfo() { return solver_info_; }

private:
  std::shared_ptr<spdlog::logger> console_;
  SharedInfo shared_info_;
  IloNumVarArray master_variables_;
  IloNumVarArray recourse_variables_;
  MasterSolverInfo solver_info_;
  std::shared_ptr<Subproblem> SP_;
  //
  BendersCustomCutCallback(){};
  //
  std::mutex mtx_;
};

inline void
BendersCustomCutCallback::invoke(const IloCplex::Callback::Context &context) {
  switch (context.getId()) {
  case IloCplex::Callback::Context::Id::Candidate:
    if (!context.isCandidatePoint()) { // The model is always bounded
      console_->error("Unbounded solution!");
      throw IloCplex::Exception(-1, "Unbounded solution");
    }
    AddLazyCuts(context);
    break;
  case IloCplex::Callback::Context::Id::Relaxation:
    // gen LRs only at the root
    if (!context.getIntInfo(IloCplex::Callback::Context::Info::NodeCount)) {
      AddLRCuts(context);
    }
    break;
  default:
    console_->error("Unexpected context!");
    throw IloCplex::Exception(-1, "Unexpected contextID");
  }
}

inline void BendersCustomCutCallback::AddLazyCuts(
    const IloCplex::Callback::Context &context) {
  mtx_.lock();
  // assert(context.getId() == IloCplex::Callback::Context::Id::Candidate);
  IloEnv env = context.getEnv();
  context.getCandidatePoint(recourse_variables_,
                            shared_info_.recourse_variables_value);
  context.getCandidatePoint(master_variables_,
                            shared_info_.master_variables_value);

  SP_->GenBendersCuts(console_, shared_info_);

  double total_violation = 0;
  uint64_t num_violated_cuts = 0;
  IloRangeArray violated_cuts(env);
  for (uint64_t sp_id = 0; sp_id < shared_info_.num_subproblems; ++sp_id) {
    if (shared_info_.retained_subproblem_ids.count(sp_id)) {
      continue;
    }

    IloExpr expr(env);
    double violation = 0;
    expr += shared_info_.subproblem_objective_value[sp_id];
    violation += shared_info_.subproblem_objective_value[sp_id];
    assert(shared_info_.dual_values[sp_id].getSize() ==
           master_variables_.getSize());
    expr -= IloScalProd(shared_info_.dual_values[sp_id],
                        shared_info_.master_variables_value);
    expr += IloScalProd(shared_info_.dual_values[sp_id], master_variables_);
    if (shared_info_.subproblem_status[sp_id]) {
      ++solver_info_.num_opt;
      expr -= recourse_variables_[sp_id];
      violation -= shared_info_.recourse_variables_value[sp_id];
    } else if (shared_info_.subproblem_objective_value[sp_id] > 0) {
      ++solver_info_.num_feas;
    } else { // SP has been infeas but unviolated (i.e., slightly infeasible)
      expr.end();
      continue;
    }
    total_violation += violation;
    if (violation > _violation_threshold) {
      ++num_violated_cuts;
      violated_cuts.add(expr <= 0);
    }
    expr.end();
  }
  if (total_violation > _violation_threshold) {
    context.rejectCandidate(violated_cuts);
  } else {
    num_violated_cuts = 0;
  }
  violated_cuts.end();
  ++solver_info_.iteration;
  {
    std::chrono::duration<float> diff =
        std::chrono::steady_clock::now() - solver_info_.start_time;
    solver_info_.duration = diff.count();
    const double _UB = context.getIncumbentObjective();
    const double _LB =
        context.getDoubleInfo(IloCplex::Callback::Context::Info::BestBound);
    const float _gap = 100 * (_UB - _LB) / (1e-75 + _UB);
    console_->info(" UB=" + ValToStr(_UB) + " LB=" + ValToStr(_LB) +
                   " Gap=" + ValToStr(_gap) +
                   "% #Cuts=" + std::to_string(num_violated_cuts) + " #Iter=" +
                   std::to_string(solver_info_.iteration) + " #Node=" +
                   std::to_string(context.getIntInfo(
                       IloCplex::Callback::Context::Info::NodeCount)) +
                   " Time=" + ValToStr(solver_info_.duration));
  }
  mtx_.unlock();
}

inline void BendersCustomCutCallback::AddLRCuts(
    const IloCplex::Callback::Context &context) {
  mtx_.lock();
  if (solver_info_.gen_user_cuts) {
    assert(context.getId() == IloCplex::Callback::Context::Id::Relaxation);
    console_->info(" -Booster still working...");
    IloEnv env = context.getEnv();
    context.getRelaxationPoint(recourse_variables_,
                               shared_info_.recourse_variables_value);
    context.getRelaxationPoint(master_variables_,
                               shared_info_.master_variables_value);

    SP_->GenBendersCuts(console_, shared_info_);
    SP_->GenAdvancedCuts(console_, shared_info_);

    uint64_t num_violated_cuts = 0;
    for (uint64_t sp_id = 0; sp_id < shared_info_.num_subproblems; ++sp_id) {
      if (shared_info_.retained_subproblem_ids.count(sp_id)) {
        continue;
      }
      double fixed_part = 0;
      const bool status = shared_info_.subproblem_status[sp_id];
      if (status) { // we have lifted only opt cuts
        ++solver_info_.num_opt;
        fixed_part = shared_info_.subproblem_objective_value[sp_id] -
                     IloScalProd(shared_info_.dual_values[sp_id],
                                 shared_info_.copied_variables_value[sp_id]);
      } else if (shared_info_.subproblem_objective_value[sp_id] >
                 _violation_threshold) { // feas cuts
        ++solver_info_.num_feas;
        fixed_part = shared_info_.subproblem_objective_value[sp_id];
        assert(fixed_part > 0);
        fixed_part -= IloScalProd(shared_info_.dual_values[sp_id],
                                  shared_info_.master_variables_value);
      } else {
        continue;
      }

      IloExpr expr(env);
      expr += fixed_part;
      expr += IloScalProd(shared_info_.dual_values[sp_id], master_variables_);
      if (status) {
        expr -= recourse_variables_[sp_id];
      }
      ++num_violated_cuts;
      context.addUserCut(expr <= 0, IloCplex::UseCutPurge, IloFalse);
      expr.end();
    }
    ++solver_info_.iteration;
    console_->info(" -Booster added " + std::to_string(num_violated_cuts) +
                   " cuts.");
    if (100 * std::fabs((context.getRelaxationObjective() -
                         solver_info_.previous_root_obj) /
                        (1e-75 + solver_info_.previous_root_obj)) <
        _min_lift_percentage) {
      console_->info(
          "  -Booster terminating because lift was " +
          std::to_string(100 *
                         std::fabs((context.getRelaxationObjective() -
                                    solver_info_.previous_root_obj) /
                                   (1e-75 + solver_info_.previous_root_obj))) +
          "%");
      solver_info_.gen_user_cuts = false;
      SP_->DeleteLRSubproblems();
      console_->info(
          " +Booster successfully terminated with " +
          ValToStr(100 * std::fabs((context.getRelaxationObjective() -
                                    solver_info_.lp_phase_LB) /
                                   (1e-75 + solver_info_.lp_phase_LB))) +
          "% lift.");
      solver_info_.LB_after_lifter = context.getRelaxationObjective();
    }
    solver_info_.previous_root_obj = context.getRelaxationObjective();
  }
  mtx_.unlock();
}

#endif
