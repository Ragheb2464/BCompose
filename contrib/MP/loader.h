#ifndef COTRIB_MP_LOADER_H
#define COTRIB_MP_LOADER_H

void SetCplexSettings(MasterModel &master_model) noexcept {
  master_model.cplex.setParam(IloCplex::Param::Emphasis::Numerical,
                              Settings::CplexSetting::NumericalEmphasis);
  master_model.cplex.setParam(IloCplex::Param::Advance,
                              Settings::CplexSetting::AdvInd);
  master_model.cplex.setParam(IloCplex::Param::MIP::Strategy::HeuristicFreq,
                              Settings::CplexSetting::HeurFreq);
  master_model.cplex.setParam(IloCplex::Param::MIP::Strategy::LBHeur,
                              Settings::CplexSetting::LBHeur);
  master_model.cplex.setParam(IloCplex::Param::RandomSeed,
                              Settings::CplexSetting::RandomSeed);
  master_model.cplex.setParam(IloCplex::Param::ClockType,
                              Settings::CplexSetting::ClockType);
  master_model.cplex.setParam(IloCplex::Param::MIP::Strategy::Probe,
                              Settings::CplexSetting::Probe);
  master_model.cplex.setParam(IloCplex::Param::Emphasis::MIP,
                              Settings::CplexSetting::MIPEmphasis);
  master_model.cplex.setParam(IloCplex::Param::RootAlgorithm,
                              Settings::CplexSetting::MP_RootAlg);
  master_model.cplex.setParam(IloCplex::Param::MIP::Cuts::Cliques,
                              Settings::CplexSetting::Cliques);
  master_model.cplex.setParam(IloCplex::Param::MIP::Cuts::Covers,
                              Settings::CplexSetting::Covers);
  master_model.cplex.setParam(IloCplex::DisjCuts,
                              Settings::CplexSetting::DisjCuts);
  master_model.cplex.setParam(IloCplex::Param::MIP::Cuts::FlowCovers,
                              Settings::CplexSetting::FlowCovers);
  master_model.cplex.setParam(IloCplex::FlowPaths,
                              Settings::CplexSetting::FlowPaths);
  master_model.cplex.setParam(IloCplex::FracCuts,
                              Settings::CplexSetting::FracCuts);
  master_model.cplex.setParam(IloCplex::Param::MIP::Cuts::GUBCovers,
                              Settings::CplexSetting::GUBCovers);
  master_model.cplex.setParam(IloCplex::ImplBd, Settings::CplexSetting::ImplBd);
  master_model.cplex.setParam(IloCplex::LiftProjCuts,
                              Settings::CplexSetting::LiftProjCuts);
  master_model.cplex.setParam(IloCplex::MIRCuts,
                              Settings::CplexSetting::MIRCuts);
  master_model.cplex.setParam(IloCplex::MCFCuts,
                              Settings::CplexSetting::MCFCuts);
  master_model.cplex.setParam(IloCplex::ZeroHalfCuts,
                              Settings::CplexSetting::ZeroHalfCuts);
  master_model.cplex.setParam(IloCplex::VarSel, Settings::CplexSetting::VarSel);
  master_model.cplex.setParam(IloCplex::NodeSel,
                              Settings::CplexSetting::NodeSel);
  master_model.cplex.setParam(IloCplex::NodeLim,
                              Settings::StoppingConditions::node_limit);
  master_model.cplex.setParam(
      IloCplex::Param::MIP::Tolerances::MIPGap,
      Settings::StoppingConditions::optimality_gap / 100.0);
  if (Settings::Solver::solver >= 2) {
    master_model.cplex.setOut(master_model.env.getNullStream());
  }
}

uint64_t ExtractVarId(const std::string &var_name) {
  std::string temp_str;
  for (uint64_t j = 0; j < var_name.size(); j++) {
    if (var_name[j] >= '0' && var_name[j] <= '9') {
      temp_str += var_name[j];
    }
  }
  assert(temp_str.length());
  return std::stoi(temp_str);
}

void GetMasterVariablesBound(const MasterModel &master_model,
                             SharedInfo &shared_info) {
  shared_info.master_variables_lb =
      IloNumArray(master_model.env, master_model.master_variables.getSize());
  shared_info.master_variables_ub =
      IloNumArray(master_model.env, master_model.master_variables.getSize());
  for (IloInt i = 0; i < master_model.master_variables.getSize(); ++i) {
    shared_info.master_variables_lb[i] =
        master_model.master_variables[i].getLB();
    shared_info.master_variables_ub[i] =
        master_model.master_variables[i].getUB();
  }
}

void Sorter(MasterModel &master_model, const SharedInfo &shared_info) {
  master_model.master_variables =
      IloNumVarArray(master_model.env, shared_info.num_master_variables);
  master_model.recourse_variables =
      IloNumVarArray(master_model.env, shared_info.num_recourse_variables);
  uint64_t var_id;
  for (int i = 0; i < master_model.variables.getSize(); ++i) {
    const std::string var_name = master_model.variables[i].getName();
    if (var_name.find("theta_") != std::string::npos) {
      var_id = ExtractVarId(var_name);
      assert(var_id <
             static_cast<uint64_t>(master_model.recourse_variables.getSize()));
      master_model.recourse_variables[var_id] = master_model.variables[i];
    } else if (var_name.find("y_") != std::string::npos) {
      var_id = ExtractVarId(var_name);
      assert(var_id <
             static_cast<uint64_t>(master_model.master_variables.getSize()));
      master_model.master_variables[var_id] = master_model.variables[i];
    }
  }
}

void GetObjectiveCoeffs(MasterModel &master_model, SharedInfo &shared_info) {
  uint64_t var_id;
  shared_info.subproblem_weight =
      IloNumArray(master_model.env, shared_info.num_recourse_variables);
  shared_info.master_variable_obj_coeff =
      IloNumArray(master_model.env, shared_info.num_master_variables);

  for (IloExpr::LinearIterator it =
           IloExpr(master_model.objective.getExpr()).getLinearIterator();
       it.ok(); ++it) {
    const std::string var_name = it.getVar().getName();
    const auto found_theta = var_name.find("theta_");
    if (found_theta != std::string::npos) {
      var_id = ExtractVarId(var_name);
      assert(var_id >= 0 &&
             var_id < static_cast<uint64_t>(
                          master_model.recourse_variables.getSize()));
      shared_info.subproblem_weight[var_id] = it.getCoef();
    } else {
      var_id = ExtractVarId(var_name);
      assert(var_id >= 0 &&
             var_id < static_cast<uint64_t>(
                          master_model.master_variables.getSize()));
      shared_info.master_variable_obj_coeff[var_id] = it.getCoef();
    }
  }
}

bool Loader(const std::shared_ptr<spdlog::logger> console,
            MasterModel &master_model, SharedInfo &shared_info,
            const std::string &current_directory) {
  master_model.env = IloEnv();
  master_model.model = IloModel(master_model.env);
  master_model.cplex = IloCplex(master_model.model);
  master_model.objective = IloObjective(master_model.env);
  master_model.constraints = IloRangeArray(master_model.env);
  master_model.opt_cuts = IloRangeArray(master_model.env);
  master_model.feas_cuts = IloRangeArray(master_model.env);
  master_model.variables = IloNumVarArray(master_model.env);
  master_model.cplex.setWarning(master_model.env.getNullStream());

  const std::string MP_model_dir = current_directory + "/opt_model_dir/MP.sav";
  master_model.cplex.importModel(master_model.model, MP_model_dir.c_str(),
                                 master_model.objective, master_model.variables,
                                 master_model.constraints);
  // getting number of master and recourse variables, we may use them for sanity
  // checks
  shared_info.num_recourse_variables = 0;
  shared_info.num_master_variables = 0;
  for (int i = master_model.variables.getSize() - 1; i >= 0; i--) {
    const std::string var_name = master_model.variables[i].getName();
    if (var_name.find("theta_") != std::string::npos) {
      ++shared_info.num_recourse_variables;
    } else if (var_name.find("y_") != std::string::npos) {
      ++shared_info.num_master_variables;
    }
  }
  //! sort variables, makes extracting values more efficient
  Sorter(master_model, shared_info);
  GetObjectiveCoeffs(master_model, shared_info);
  GetMasterVariablesBound(master_model, shared_info);
  SetCplexSettings(master_model);

  if (master_model.cplex.isMIP()) {
    console->error("Loaded MP is MIP");
    exit(0);
  }
  if (!master_model.cplex.solve()) {
    console->error("   Status of the loaded master problem is " +
                   std::to_string(master_model.cplex.getStatus()));
    console->error(
        "Please make sure that your MP.sav model is corrrect and try again!");
    exit(0);
  }
  console->info("    Master model successfully loaded.");

  return true;
}
#endif
