#ifndef CONTRIB_SP_UTILS_LR_CUTS_H
#define CONTRIB_SP_UTILS_LR_CUTS_H

#include "../../shared_info/structures.h"
#include "level.h"

double GetCutViolation(const IloNumArray &master_solution,
                       const IloNumArray &sp_solution,
                       const SharedInfo *shared_info, const uint64_t sp_id) {
  double violation = -shared_info->recourse_variables_value[sp_id];
  violation += IloScalProd(master_solution, shared_info->dual_values[sp_id]);
  violation -= IloScalProd(sp_solution, shared_info->dual_values[sp_id]);
  violation += shared_info->subproblem_objective_value[sp_id];

  return violation;
}

void UpdateObjCoeff(SubproblemModel *mip_sp_model,
                    const SharedInfo *shared_info, const uint64_t sp_id) {
  mip_sp_model->objective.setExpr(mip_sp_model->regular_objective.getExpr() -
                                  IloScalProd(mip_sp_model->copied_variables,
                                              shared_info->dual_values[sp_id]));
}

void Solve(SubproblemModel *mip_sp_model) {
  assert(mip_sp_model->cplex.solve());
}

double GetcTx(const SubproblemModel *mip_sp_model) {
  return mip_sp_model->cplex.getValue(
      mip_sp_model->regular_objective.getExpr());
}

void ExtractVariableValues(const SubproblemModel *mip_sp_model,
                           SharedInfo *shared_info, const uint64_t sp_id) {
  mip_sp_model->cplex.getValues(shared_info->copied_variables_value[sp_id],
                                mip_sp_model->copied_variables);
}

void UpdateGradients(IloNumArray &gradient, const SharedInfo *shared_info,
                     const uint64_t sp_id) {
  for (IloInt i = 0; i < shared_info->master_variables_value.getSize(); i++)
    gradient[i] = shared_info->master_variables_value[i] -
                  shared_info->copied_variables_value[sp_id][i];
}

double GetUB(SubproblemModel *lp_sp_model, const IloNumArray &var_val) {
  double ub = IloInfinity;
  lp_sp_model->NAC_constraints.setBounds(var_val, var_val);
  if (lp_sp_model->cplex.solve()) {
    ub = lp_sp_model->cplex.getObjValue();
  }
  return ub;
}

void GenLRCuts(SubproblemModel *mip_sp_model, SharedInfo *shared_info,
               std::vector<std::vector<int>> *copy_sol_tmp_vec,
               const uint64_t sp_id) {
  assert(shared_info->subproblem_status[sp_id]);
  double cut_violation_0 =
      GetCutViolation(shared_info->master_variables_value,
                      shared_info->master_variables_value, shared_info, sp_id);
  IloNumArray best_copied_variables_value = IloNumArray(
      mip_sp_model->env, shared_info->master_variables_value.getSize());

  Level level_method;
  level_method.Initializer(shared_info->master_variables_value.getSize());
  level_method.SetInitialDualValues(shared_info->dual_values[sp_id]);
  IloNumArray gradient = IloNumArray(
      mip_sp_model->env, shared_info->master_variables_value.getSize());
  IloNum level, ub, lb, lambda = 1e-5, best_lb = -IloInfinity,
                        delta = IloInfinity, best_ub = IloInfinity;
  int iter = 0;
  while (true) {
    UpdateObjCoeff(mip_sp_model, shared_info, sp_id);
    Solve(mip_sp_model);
    ExtractVariableValues(mip_sp_model, shared_info, sp_id);
    {
      std::vector<int> aux_vec;
      for (IloInt i = 0;
           i < shared_info->copied_variables_value[sp_id].getSize(); ++i) {
        aux_vec.push_back(shared_info->copied_variables_value[sp_id][i]);
      }
      copy_sol_tmp_vec->emplace_back(aux_vec);
    }

    double cut_violation = GetCutViolation(
        shared_info->master_variables_value,
        shared_info->copied_variables_value[sp_id], shared_info, sp_id);

    lb = mip_sp_model->cplex.getBestObjValue() +
         IloScalProd(shared_info->dual_values[sp_id],
                     shared_info->master_variables_value);

    UpdateGradients(gradient, shared_info, sp_id);
    bool update_proj_model = false;
    if (lb > best_lb) {
      best_lb = lb;
      level_method.UpdateBestDualValue(shared_info->dual_values[sp_id]);
      mip_sp_model->cplex.getValues(best_copied_variables_value,
                                    mip_sp_model->copied_variables);
      shared_info->subproblem_objective_value[sp_id] = GetcTx(mip_sp_model);
      update_proj_model = true;
    }
    // break;
    best_ub = shared_info->subproblem_objective_value[sp_id];

    delta = best_ub - best_lb;
    level = best_ub - lambda * delta;

    if (std::fabs(IloSum(gradient)) < 1e-7 ||
        (cut_violation - 1e-7 <= cut_violation_0 && iter >= 1) ||
        iter >= _lifter_aggressiveness) {
      break;
    }
    cut_violation_0 = cut_violation;
    if (update_proj_model) {
      level_method.UpdateObjective();
      level_method.UpdateConstraints(level);
    }

    level_method.AddLevelConstraint(lb, level, gradient);
    level_method.Solve();
    level_method.GetDualValues(shared_info->dual_values[sp_id]);
    ++iter;
  }  // while
  level_method.GetBestDualValues(shared_info->dual_values[sp_id]);
  for (IloInt i = 0; i < best_copied_variables_value.getSize(); ++i) {
    shared_info->copied_variables_value[sp_id][i] =
        best_copied_variables_value[i];
  }
  level_method.EndEnv();
}

#endif
