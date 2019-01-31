#ifndef CONTRIB_SP_UTIL_CLASSICIAL_CUTS_H
#define CONTRIB_SP_UTIL_CLASSICIAL_CUTS_H

void GenClassicalCuts(SubproblemModel *sp_model, SharedInfo *shared_info,
                      const uint64_t sp_id) {
  shared_info->subproblem_status[sp_id] = sp_model->cplex.solve();
  if (shared_info->subproblem_status[sp_id]) {  // opt
    sp_model->cplex.getDuals(shared_info->dual_values[sp_id],
                             sp_model->NAC_constraints);
    shared_info->subproblem_objective_value[sp_id] =
        sp_model->cplex.getObjValue();
  } else {  // feas
    // set the feas cut generator
    sp_model->slack_variables.setBounds(sp_model->slacks_lb,
                                        sp_model->slacks_ub);

    sp_model->objective.setExpr(sp_model->slack_objective);
    if (!sp_model->cplex.solve()) {
      std::cout << "ERROR: Feasibility problem is infeasible!" << std::endl;
      sp_model->cplex.exportModel(("SP_" + std::to_string(sp_id)).c_str());
      exit(1);
    }
    shared_info->subproblem_objective_value[sp_id] =
        sp_model->cplex.getObjValue();
    if (shared_info->subproblem_objective_value[sp_id] <= -1e-7) {
      sp_model->cplex.exportModel("SP_.lp");
      std::cout << "ERROR:: Objective of the feasibility problem is "
                << shared_info->subproblem_objective_value[sp_id]
                << "!. Most likely numeric issue." << std::endl;
      exit(1);
    }
    sp_model->cplex.getDuals(shared_info->dual_values[sp_id],
                             sp_model->NAC_constraints);
    // reset the problem to the regular for opt cut generator
    sp_model->slack_variables.setBounds(sp_model->slacks_lb,
                                        sp_model->slacks_lb);
    sp_model->objective.setExpr(sp_model->regular_objective);
  }
}

#endif
