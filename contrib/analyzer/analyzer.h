#ifndef CONTRIB_ANALYZER_H
#define CONTRIB_ANALYZER_H

/*!
  This is provide the user with a set of useful metrics to impprove the
  performance, like suggesting to used primalheuristic because of many infeas
  cuts or use VIs bbecause of waek root node bound
!*/

//! TODO

void AnalyzeSubproblems(const std::shared_ptr<spdlog::logger> console,
                        SharedInfo &shared_info) {
  shared_info.problem_info.is_determinstic =
      true ? shared_info.num_subproblems == 1 : false;
  if (false && Settings::GlobalScenarios::num_retention +
                   Settings::GlobalScenarios::num_creation) {
    //! obj
    for (uint64_t sp_id = 1; sp_id < shared_info.num_subproblems; ++sp_id) {
      for (uint64_t var_id = 0;
           var_id < shared_info.subproblem_data[sp_id].obj_coeffs.getSize();
           ++var_id) {
        if (shared_info.subproblem_data[sp_id].obj_coeffs[var_id] !=
            shared_info.subproblem_data[0].obj_coeffs[var_id]) {
          shared_info.problem_info.is_obj_coeff_stochastic = true;
          break;
        }
      }
      if (shared_info.problem_info.is_obj_coeff_stochastic) {
        break;
      }
    }
    //! RHS
    for (uint64_t sp_id = 1; sp_id < shared_info.num_subproblems; ++sp_id) {
      for (uint64_t con_id = 0;
           con_id < shared_info.subproblem_data[sp_id].constraint_lb.getSize();
           ++con_id) {
        if (shared_info.subproblem_data[sp_id].constraint_lb[con_id] !=
                shared_info.subproblem_data[0].constraint_lb[con_id] ||
            shared_info.subproblem_data[sp_id].constraint_ub[con_id] !=
                shared_info.subproblem_data[0].constraint_ub[con_id]) {
          shared_info.problem_info.is_RHS_stochastic = true;
          break;
        }
      }
      if (shared_info.problem_info.is_RHS_stochastic) {
        break;
      }
    }
    //! Con Coeff Matrix
    for (uint64_t sp_id = 1; sp_id < shared_info.num_subproblems; ++sp_id) {
      for (uint64_t con_id = 0;
           con_id <
           shared_info.subproblem_data[sp_id].constraint_coeffs.getSize();
           ++con_id) {
        for (uint64_t var_id = 0; var_id < shared_info.subproblem_data[sp_id]
                                               .constraint_coeffs[con_id]
                                               .getSize();
             ++var_id) {
          if (shared_info.subproblem_data[sp_id]
                  .constraint_coeffs[con_id][var_id] !=
              shared_info.subproblem_data[0]
                  .constraint_coeffs[con_id][var_id]) {
            shared_info.problem_info.is_constraint_coeff_uncertain = true;
            break;
          }
        }
        if (shared_info.problem_info.is_constraint_coeff_uncertain) {
          break;
        }
      }
      if (shared_info.problem_info.is_constraint_coeff_uncertain) {
        break;
      }
    }

    // clean up
    for (uint64_t sp_id = 1; sp_id < shared_info.num_subproblems; ++sp_id) {
      if (!shared_info.problem_info.is_obj_coeff_stochastic) {
        shared_info.subproblem_data[sp_id].obj_coeffs.end();
      }
      if (!shared_info.problem_info.is_RHS_stochastic) {
        shared_info.subproblem_data[sp_id].constraint_lb.end();
        shared_info.subproblem_data[sp_id].constraint_ub.end();
      }
      if (!shared_info.problem_info.is_constraint_coeff_uncertain) {
        shared_info.subproblem_data[sp_id].constraint_coeffs.end();
      }
    }
    // print info
    if (shared_info.problem_info.is_determinstic) {
      console->info("   The problem is determinstic.");
    } else {
      console->info("   The problem is stochastic with unccertainty in");
      if (shared_info.problem_info.is_obj_coeff_stochastic) {
        console->info("    -objective");
      }
      if (shared_info.problem_info.is_RHS_stochastic) {
        console->info("    -RHS");
      }
      if (shared_info.problem_info.is_constraint_coeff_uncertain) {
        console->info("    -constraint matrix");
      }
    }
  }
}
#endif
