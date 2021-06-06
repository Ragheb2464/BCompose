#ifndef CONTRIB_SP_UTIL_CLASSICIAL_CUTS_H
#define CONTRIB_SP_UTIL_CLASSICIAL_CUTS_H

void GenClassicalCuts(SubproblemModel *sp_model, SharedInfo *shared_info_,
                      const uint64_t sp_id) {
    if (sp_model->cplex.solve()) {  // opt
        shared_info_->subproblem_status[sp_id] = 1;
        sp_model->cplex.getDuals(shared_info_->dual_values[sp_id],
                                 sp_model->NAC_constraints);
        shared_info_->subproblem_objective_value[sp_id] =
                sp_model->cplex.getObjValue();
    } else if (!_is_complete_recourse) {  // feas
        shared_info_->subproblem_status[sp_id] = 0;
        // set the feas cut generator
        sp_model->slack_variables.setBounds(sp_model->slacks_lb,
                                            sp_model->slacks_ub);
        sp_model->objective.setExpr(sp_model->slack_objective);
        if (!sp_model->cplex.solve()) {
            std::cout << "ERROR: Feasibility problem is infeasible!" << std::endl;
            sp_model->cplex.exportModel(("SP_" + std::to_string(sp_id) + ".lp").c_str());
            std::abort();
        }
        shared_info_->subproblem_objective_value[sp_id] =
                sp_model->cplex.getObjValue();
        if (shared_info_->subproblem_objective_value[sp_id] <= -1e-7) {
            sp_model->cplex.exportModel("SP_.lp");
            std::cout << "ERROR:: Objective of the feasibility problem is "
                      << shared_info_->subproblem_objective_value[sp_id]
                      << "!. Most likely numeric issue." << std::endl;
            std::abort();
        }
        sp_model->cplex.getDuals(shared_info_->dual_values[sp_id],
                                 sp_model->NAC_constraints);
        // reset the problem to the regular for opt cut generator
        sp_model->slack_variables.setBounds(sp_model->slacks_lb,
                                            sp_model->slacks_lb);
        sp_model->objective.setExpr(sp_model->regular_objective);
    } else {
        std::cout
                << "According to the settings, the subproblem must always be feasible"
                << std::endl;
        sp_model->cplex.exportModel("SP_.lp");
        std::abort();
    }
}

#endif
