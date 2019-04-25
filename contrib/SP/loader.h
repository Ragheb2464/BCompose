#ifndef CONTRIB_SP_LOADER_H
#define CONTRIB_SP_LOADER_H

// #include <omp.h>
#include "../control/solver_settings.h"
#include "../shared_info/structures.h"
/*!
  This function modifies the cplex settings for the loaded subproblem
  formulation.
!*/
void SetSPCplexSettings(SubproblemModel &subproblem_model) noexcept(true) {
  if (Settings::Parallelization::num_proc > 1 ||
      Settings::Parallelization::num_master_procs > 1) {
    subproblem_model.cplex.setParam(IloCplex::Param::Threads, 1);
  }
  subproblem_model.cplex.setParam(IloCplex::RootAlg,
                                  Settings::CplexSetting::SP_RootAlg);
  subproblem_model.cplex.setParam(IloCplex::Param::Emphasis::Numerical,
                                  Settings::CplexSetting::NumericalEmphasis);

  // subproblem_model.cplex.setParam(
  //     IloCplex::Param::Simplex::Tolerances::Optimality,
  //     1e-9);
}

/*Extracting the copied variables nd those which have been fixed  */
void GetCopiedAndFixedVars(SharedInfo &shared_info,
                           SubproblemModel &subproblem_model) {
  assert(static_cast<uint64_t>(subproblem_model.copied_variables.getSize()) ==
         shared_info.num_master_variables);
  IloInt num_master_variables = 0;
  uint64_t var_id;
  for (IloInt i = 0; i < subproblem_model.variables.getSize(); ++i) {
    std::string var_name{subproblem_model.variables[i].getName()};
    std::size_t found = var_name.find("z_");
    if (found != std::string::npos) {
      ++num_master_variables;
      var_id = std::stoi(var_name.erase(found, 2));
      subproblem_model.copied_variables[var_id] = subproblem_model.variables[i];
      // check if the pre-solve has fixed the var
      double var_lb = subproblem_model.copied_variables[var_id].getLB();
      double var_ub = subproblem_model.copied_variables[var_id].getUB();
      if (std::round(var_lb) == std::round(var_ub)) {
        subproblem_model.copied_variables[var_id].setBounds(
            shared_info.master_variables_lb[var_id],
            shared_info.master_variables_ub[var_id]);
      }
    }
  }  // for
  assert(num_master_variables == subproblem_model.copied_variables.getSize());
}

/* This funnc is used to remove master variables from the obbjective*/
void DropMasterVarsFromObj(SubproblemModel &subproblem_model) {
  uint64_t num_master_variables = 0;
  IloExpr expr(subproblem_model.env);
  for (IloExpr::LinearIterator it =
           IloExpr(subproblem_model.objective.getExpr()).getLinearIterator();
       it.ok(); ++it) {
    const std::string var_name = it.getVar().getName();
    if (var_name.find("z_") != std::string::npos) {
      ++num_master_variables;
      continue;
    }
    expr += it.getCoef() * it.getVar();
  }
  subproblem_model.objective.setExpr(expr);
  expr.end();
}
/*!
    This function adds slacks to a given constraint
!*/
void AddSlack(const uint64_t con_id, SubproblemModel &subproblem_model) {
  const IloNum con_lb = subproblem_model.constraints[con_id].getLB();
  const IloNum con_ub = subproblem_model.constraints[con_id].getUB();

  if (con_lb == con_ub) {  // cons + s_1 - s_2 = b
    for (int s = 0; s < 2; s++) {
      subproblem_model.slack_variables.add(
          IloNumVar(subproblem_model.env, 0, IloInfinity));
      // uint64_t slack_id = subproblem_model.slack_variables.getSize() - 1;
      // subproblem_model.slack_variables[slack_id].setName(
      //     ("slack_" + std::to_string(slack_id)).c_str());
      subproblem_model.slacks_lb.add(0);
      subproblem_model.slacks_ub.add(IloInfinity);
    }
    uint64_t slack_id = subproblem_model.slack_variables.getSize() - 1;
    subproblem_model.constraints[con_id].setLinearCoef(
        subproblem_model.slack_variables[slack_id - 1], 1);
    subproblem_model.constraints[con_id].setLinearCoef(
        subproblem_model.slack_variables[slack_id], -1);

  } else if (con_lb == -IloInfinity) {  // cons -  s_1 <= b
    subproblem_model.slack_variables.add(
        IloNumVar(subproblem_model.env, 0, IloInfinity));
    uint64_t slack_id = subproblem_model.slack_variables.getSize() - 1;
    // subproblem_model.slack_variables[slack_id].setName(
    //     ("slack_" + std::to_string(slack_id)).c_str());
    subproblem_model.slacks_lb.add(0);
    subproblem_model.slacks_ub.add(IloInfinity);

    subproblem_model.constraints[con_id].setLinearCoef(
        subproblem_model.slack_variables[slack_id], -1);
  } else if (con_ub == IloInfinity) {  // cons + s_1 >= b
    subproblem_model.slack_variables.add(
        IloNumVar(subproblem_model.env, 0, IloInfinity));
    uint64_t slack_id = subproblem_model.slack_variables.getSize() - 1;
    // subproblem_model.slack_variables[slack_id].setName(
    //     ("slack_" + std::to_string(slack_id)).c_str());
    subproblem_model.slacks_lb.add(0);
    subproblem_model.slacks_ub.add(IloInfinity);
    subproblem_model.constraints[con_id].setLinearCoef(
        subproblem_model.slack_variables[slack_id], 1);
  } else {
    const bool constraint_type_is_unkown = false;
    assert(!constraint_type_is_unkown);
  }

  assert(subproblem_model.slacks_lb.getSize() ==
             subproblem_model.slacks_lb.getSize() &&
         subproblem_model.slacks_lb.getSize() ==
             subproblem_model.slack_variables.getSize());
}

/*!
  This function checks if number of NAC constraints iin each subproblem is the
  same as the size of the master variables. At the same time, it extracts the
  NAC constraints and reorder them which we update repeatedly.
!*/
void HealthCheckAndNACExtraction(SubproblemModel &subproblem_model,
                                 const SharedInfo &shared_info) {
  {
    if (!ProblemSpecificSettings::ProblemProperties::is_complete_recourse) {
      for (IloInt con_id = 0; con_id < subproblem_model.constraints.getSize();
           ++con_id) {
        // std::string constraint_name =
        //     subproblem_model.constraints[con_id].getName();
        // std::size_t found = constraint_name.find("NAC_");
        // assert(found == std::string::npos); // check that  model does not
        // already has NAC constraints
        // add a slack, to make it possible to gen feas cuts we need to know how
        // to add the slack
        AddSlack(con_id, subproblem_model);
      }
    }
    // build an objective only of slack variables for quick obj update
    {
      subproblem_model.slack_objective.setExpr(
          IloSum(subproblem_model.slack_variables));
      // keep the regular objective for opt cut gen
      subproblem_model.regular_objective.setExpr(subproblem_model.objective);
      // keep them fixed to until you need them
      subproblem_model.slack_variables.setBounds(subproblem_model.slacks_lb,
                                                 subproblem_model.slacks_lb);
    }
  }

  {  // adding NAC to the model
    IloInt var_id;
    for (IloInt i = 0; i < subproblem_model.copied_variables.getSize(); ++i) {
      std::string var_name = subproblem_model.copied_variables[i].getName();
      std::size_t found = var_name.find("z_");
      assert(found != std::string::npos);
      {
        var_id = std::stoi(var_name.erase(found, 2));
        assert(var_id < subproblem_model.NAC_constraints.getSize());

        subproblem_model.NAC_constraints[var_id] = IloRange(
            subproblem_model.env, subproblem_model.copied_variables[i].getUB(),
            subproblem_model.copied_variables[i],
            subproblem_model.copied_variables[i].getUB());
        // subproblem_model.constraints[con_id];
        subproblem_model.NAC_constraints[var_id].setName(
            ("NAC_" + std::to_string(var_id)).c_str());
      }
    }
    assert(shared_info.num_master_variables ==
           static_cast<uint64_t>(subproblem_model.NAC_constraints.getSize()));
    subproblem_model.model.add(subproblem_model.NAC_constraints);
  }

  // { // health check and warn if the probleem seems unhealthy
  //   if (!subproblem_model.cplex.solve()) {
  //     console->warn("  Status of the loaded subproblem is " +
  //                   std::to_string(subproblem_model.cplex.getStatus()));
  //     // exit(0);
  //   }
  // }
}

/*
  This purturbs the RHS for parto cut generator
*/
void PerturbRHS(SubproblemModel &subproblem_model) {
  assert(false);
  const double alpha = Settings::ParetoCuts::perturbation_weight;
  for (IloInt con_id = 0; con_id < subproblem_model.constraints.getSize();
       ++con_id) {
    // const std::string con_name =
    // subproblem_model.constraints[con_id].getName();
    /*  if (con_name.find("Reg_") != std::string::npos) */ {
      const double lb = subproblem_model.constraints[con_id].getLB();
      const double ub = subproblem_model.constraints[con_id].getUB();
      // std::cout << lb << " " << ub << std::endl;
      // if (lb == ub) {
      subproblem_model.constraints[con_id].setBounds(lb - alpha * lb,
                                                     ub - alpha * ub);
      // } else {
      //   subproblem_model.constraints[con_id].setBounds(lb - alpha * lb,
      //                                                  ub - alpha * ub);
      // }
      // ++t;
    }
  }
}

/*!
  This func extrct all the data in the model, used to create art SPs
!*/
void GetObjectiveCoeffs(SubproblemData &data, SubproblemModel &model) {
  // uint64_t var_id;
  data.obj_coeffs = IloNumArray(model.env);
  for (IloExpr::LinearIterator it =
           IloExpr(model.objective.getExpr()).getLinearIterator();
       it.ok(); ++it) {
    const std::string var_name = it.getVar().getName();
    assert(var_name.find("z_") == std::string::npos);
    data.obj_coeffs.add(it.getCoef());
  }
}
uint64_t GetNumRegularConstraints(SubproblemModel &subproblem_model) {
  uint64_t num_con_without_cplex_cuts_or_NAC = 0;
  for (IloInt con_id = 0; con_id < subproblem_model.constraints.getSize();
       ++con_id) {
    const std::string con_name = subproblem_model.constraints[con_id].getName();
    if (con_name.find("Reg_") != std::string::npos) {
      ++num_con_without_cplex_cuts_or_NAC;
    }
  }
  return num_con_without_cplex_cuts_or_NAC;
}
void GetRHS(SubproblemData &data, SubproblemModel &subproblem_model) {
  uint64_t num_con_without_cplex_cuts_or_NAC =
      GetNumRegularConstraints(subproblem_model);
  data.constraint_lb =
      IloNumArray(subproblem_model.env, num_con_without_cplex_cuts_or_NAC);
  data.constraint_ub =
      IloNumArray(subproblem_model.env, num_con_without_cplex_cuts_or_NAC);

  uint64_t t = 0;
  for (IloInt con_id = 0; con_id < subproblem_model.constraints.getSize();
       ++con_id) {
    const std::string con_name = subproblem_model.constraints[con_id].getName();
    if (con_name.find("Reg_") != std::string::npos) {
      data.constraint_lb[con_id] = subproblem_model.constraints[con_id].getLB();
      data.constraint_ub[con_id] = subproblem_model.constraints[con_id].getUB();
      ++t;
    }
  }
  assert(t == num_con_without_cplex_cuts_or_NAC);
}
void GetConstraintMatrix(SubproblemData &data,
                         SubproblemModel &subproblem_model) {
  uint64_t num_con_without_cplex_cuts_or_NAC =
      GetNumRegularConstraints(subproblem_model);

  data.constraint_coeffs =
      IloNumArray2(subproblem_model.env, num_con_without_cplex_cuts_or_NAC);

  uint64_t t = 0;
  for (IloInt con_id = 0; con_id < subproblem_model.constraints.getSize();
       ++con_id) {
    const std::string con_name = subproblem_model.constraints[con_id].getName();
    if (con_name.find("Reg_") != std::string::npos) {
      data.constraint_coeffs[t] = IloNumArray(subproblem_model.env);
      for (IloExpr::LinearIterator it =
               IloExpr(subproblem_model.constraints[con_id].getExpr())
                   .getLinearIterator();
           it.ok(); ++it) {
        data.constraint_coeffs[t].add(it.getCoef());
      }
      ++t;
    }
  }
}

/*!
  This function loads the subproblem formulations
!*/
bool Loader(const std::shared_ptr<spdlog::logger> console,
            std::vector<SubproblemModel> &subproblem_model,
            SharedInfo &shared_info, const std::string &current_directory) {
  if (Settings::GlobalScenarios::num_retention +
      Settings::GlobalScenarios::num_creation) {
    shared_info.subproblem_data.resize(shared_info.num_recourse_variables);
  }
  for (uint64_t sp_id = 0; sp_id < shared_info.num_recourse_variables;
       sp_id++) {
    subproblem_model.emplace_back(SubproblemModel());
  }
  //! TODO: Make this part parallel
  // const std::size_t num_proc = omp_get_max_threads();
  // omp_set_num_threads(num_proc);
  // #pragma omp parallel
  {
    // #pragma omp for private(sp_id) schedule(dynamic)
    for (uint64_t sp_id = 0; sp_id < shared_info.num_recourse_variables;
         sp_id++) {
      {  // setting up the model
        subproblem_model[sp_id].env = IloEnv();
        subproblem_model[sp_id].model = IloModel(subproblem_model[sp_id].env);
        subproblem_model[sp_id].cplex = IloCplex(subproblem_model[sp_id].model);

        subproblem_model[sp_id].objective =
            IloObjective(subproblem_model[sp_id].env);
        subproblem_model[sp_id].regular_objective =
            IloObjective(subproblem_model[sp_id].env);
        subproblem_model[sp_id].slack_objective =
            IloObjective(subproblem_model[sp_id].env);

        subproblem_model[sp_id].constraints =
            IloRangeArray(subproblem_model[sp_id].env);
        subproblem_model[sp_id].NAC_constraints = IloRangeArray(
            subproblem_model[sp_id].env, shared_info.num_master_variables);

        subproblem_model[sp_id].variables =
            IloNumVarArray(subproblem_model[sp_id].env);
        subproblem_model[sp_id].copied_variables = IloNumVarArray(
            subproblem_model[sp_id].env, shared_info.num_master_variables);
        subproblem_model[sp_id].slack_variables =
            IloNumVarArray(subproblem_model[sp_id].env);

        subproblem_model[sp_id].slacks_lb =
            IloNumArray(subproblem_model[sp_id].env);
        subproblem_model[sp_id].slacks_ub =
            IloNumArray(subproblem_model[sp_id].env);
      }

      {
        subproblem_model[sp_id].cplex.setOut(
            subproblem_model[sp_id].env.getNullStream());
        subproblem_model[sp_id].cplex.setWarning(
            subproblem_model[sp_id].env.getNullStream());
        const std::string model_dir = current_directory + "/opt_model_dir/SP_" +
                                      std::to_string(sp_id) + ".sav";
        subproblem_model[sp_id].cplex.importModel(
            subproblem_model[sp_id].model, model_dir.c_str(),
            subproblem_model[sp_id].objective,
            subproblem_model[sp_id].variables,
            subproblem_model[sp_id].constraints);
      }
      GetCopiedAndFixedVars(shared_info, subproblem_model[sp_id]);
      DropMasterVarsFromObj(subproblem_model[sp_id]);
      HealthCheckAndNACExtraction(subproblem_model[sp_id], shared_info);

      if (false && Settings::GlobalScenarios::num_creation) {
        // we will only load the basic information, nothing for cplex
        // cuts or NAC.
        shared_info.subproblem_data[sp_id] = SubproblemData();
        GetObjectiveCoeffs(shared_info.subproblem_data[sp_id],
                           subproblem_model[sp_id]);
        GetRHS(shared_info.subproblem_data[sp_id], subproblem_model[sp_id]);
        GetConstraintMatrix(shared_info.subproblem_data[sp_id],
                            subproblem_model[sp_id]);
        //! Check uniformity
        assert(shared_info.subproblem_data[0].obj_coeffs.getSize() ==
               shared_info.subproblem_data[sp_id].obj_coeffs.getSize());
        assert(shared_info.subproblem_data[0].constraint_lb.getSize() ==
               shared_info.subproblem_data[sp_id].constraint_lb.getSize());
        assert(shared_info.subproblem_data[0].constraint_coeffs.getSize() ==
               shared_info.subproblem_data[sp_id].constraint_coeffs.getSize());

        for (uint64_t con_id = 0;
             con_id <
             shared_info.subproblem_data[0].constraint_coeffs.getSize();
             ++con_id) {
          if (shared_info.subproblem_data[0]
                  .constraint_coeffs[con_id]
                  .getSize() != shared_info.subproblem_data[sp_id]
                                    .constraint_coeffs[con_id]
                                    .getSize()) {
            std::cout << "/* sh0o0t SPs are does not have the same struct, "
                         "turn of articulations*/"
                      << '\n'
                      << std::flush;
            exit(0);
          }
        }
      }
      SetSPCplexSettings(subproblem_model[sp_id]);
    }
  }
  console->info("   Subproblems loaded successfully.");

  return true;
}

/*!
  This func gets total number of the subproblems in the
model_directory
!*/
uint64_t GetSubproblemCount() {
  uint64_t number_of_files = 0;
  const int possible_max = 1e5;  // some number you can expect.
  for (int s = 0; s < possible_max; s++) {
    std::string file_name = "opt_model_dir/SP_" + std::to_string(s) + ".sav";
    std::fstream file;
    file.open(file_name);
    if (file) {
      number_of_files++;
    } else {
      break;
    }
  }
  return number_of_files;
}

#endif
