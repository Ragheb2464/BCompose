#ifndef CONTRIB_MP_UTILES_PD_H
#define CONTRIB_MP_UTILES_PD_H

#include "../../shared_info/structures.h"
#include "../structures.h"

/*!
  This func gets subproblem id and addes it to (master) model
!*/
void AddSPToMP(const uint64_t sp_id, MasterModel &mp_model,
               const std::string &current_directory) {
  IloEnv env = mp_model.env;
  IloModel model = IloModel(env);
  IloCplex cplex = IloCplex(model);
  IloObjective objective = IloObjective(env);
  IloRangeArray constraints = IloRangeArray(env);
  IloNumVarArray variables = IloNumVarArray(env);

  cplex.setOut(env.getNullStream());
  cplex.setWarning(env.getNullStream());
  const std::string model_dir =
      current_directory + "/opt_model_dir/SP_" + std::to_string(sp_id) + ".sav";
  cplex.importModel(model, model_dir.c_str(), objective, variables,
                    constraints);

  mp_model.model.add(variables);
  // I have to remove fTy part from subproblems objective

  {  // adding theta>=obj
    uint64_t num_master_variables = 0;
    IloExpr expr(env);
    for (IloExpr::LinearIterator it =
             IloExpr(objective.getExpr()).getLinearIterator();
         it.ok(); ++it) {
      const std::string var_name = it.getVar().getName();
      if (var_name.find("z_") != std::string::npos) {
        ++num_master_variables;
        continue;
      }
      expr += it.getCoef() * it.getVar();
    }
    expr -= mp_model.recourse_variables[sp_id];
    mp_model.model.add(IloRange(env, -IloInfinity, expr, 0));
    // assert(!num_master_variables ||
    //        num_master_variables == mp_model.master_variables.getSize());
    // subproblem_model.objective.setExpr(expr);
    expr.end();
  }
  {  // enforcing z=y
    uint64_t var_id;
    uint64_t NAC_count = 0;
    for (IloInt i = 0; i < variables.getSize(); ++i) {
      std::string var_name = variables[i].getName();
      std::size_t found = var_name.find("z_");
      if (found != std::string::npos) {
        var_id = std::stoi(var_name.erase(found, 2));
        assert(var_id <
               static_cast<uint64_t>(mp_model.master_variables.getSize()));
        constraints.add(IloRange(
            env, 0, variables[i] - mp_model.master_variables[var_id], 0));
        ++NAC_count;
      }
    }
    {  // relax z vars if they are int
      if (cplex.isMIP()) {
        mp_model.model.add(IloConversion(env, variables, ILOFLOAT));
      }
    }
    assert(NAC_count ==
           static_cast<uint64_t>(mp_model.master_variables.getSize()));
  }
  mp_model.model.add(constraints);
  mp_model.cplex.exportModel("PD.lp");
}

void RandomSelection(SharedInfo &shared_info, const uint64_t num_retention) {
  // assert(shared_info.num_subproblems >= num_retention);
  uint64_t t = 0;
  assert(!shared_info.retained_subproblem_ids.size());
  while (shared_info.retained_subproblem_ids.size() < num_retention) {
    uint64_t chosen_id = RandInt(0, shared_info.num_subproblems - 1);
    assert(chosen_id < shared_info.num_subproblems);
    shared_info.retained_subproblem_ids.insert(chosen_id);
    if (shared_info.retained_subproblem_ids.size() >= num_retention) {
      break;
    }
    ++t;
    if (t >= 100 * num_retention) {
      assert(shared_info.retained_subproblem_ids.size() < num_retention);
      std::cout << "Couldn't select the requested number of SPs in "
                << 100 * num_retention << " iterations" << '\n';
      std::cout << "  --Maybe reduce the requested number!" << '\n';
      exit(0);
    }
  }

  assert(shared_info.retained_subproblem_ids.size() == num_retention);
}

void RowCovering() {}

/*
  This function uses the information obtained from lifting the SPs to select the
  global scenarios for retention. In doing so, it orders the SPs based on their
  objective value and select the ones with highest value. We also multiply the
  obj to the probablity to chose the true important one, not one with probablity
  0 and high cost.
*/
void MaxCost(SharedInfo &shared_info, const uint64_t num_retention,
             const std::string &current_directory) {
  std::set<std::pair<double, uint64_t>> obj_sp_pair;
  const std::string path_to_SP_data_file =
      current_directory + "/contrib/pre_processor/SP_info.txt";
  std::fstream file;  //(path_to_SP_data_file, std::ofstream::in);
  file.open(path_to_SP_data_file);
  assert(file.is_open());
  std::string val;
  uint64_t sp_id;
  while (file >> val) {  // file is formated as SP_id = obj_val
    if (val == "=") {
      continue;
    } else if (val[0] == 'S') {
      sp_id = std::stoi(val.erase(0, 3));
    } else if (val[0] >= '-' ||
               (val[0] >= '0' &&
                val[0] <= '9')) {  // be careful of negative values
      const auto &res = obj_sp_pair.insert(std::make_pair(
          -std::fabs(shared_info.subproblem_weight[sp_id] * std::stod(val)),
          sp_id));  // reversing the order to decending
      assert(res.second);
    } else {
      const bool malformed_txt_file = false;
      assert(malformed_txt_file);
    }
  }
  file.close();
  assert(obj_sp_pair.size() == shared_info.num_subproblems);
  // pick the
  assert(!shared_info.retained_subproblem_ids.size());
  for (const auto pair : obj_sp_pair) {
    const auto &res = shared_info.retained_subproblem_ids.insert(pair.second);
    assert(res.second);
    if (shared_info.retained_subproblem_ids.size() >= num_retention) {
      break;
    }
  }
  assert(shared_info.retained_subproblem_ids.size() == num_retention);
}
void MinCost(SharedInfo &shared_info, const uint64_t num_retention,
             const std::string &current_directory) {
  std::set<std::pair<double, uint64_t>> obj_sp_pair;
  const std::string path_to_SP_data_file =
      current_directory + "/contrib/pre_processor/SP_info.txt";
  std::fstream file;
  file.open(path_to_SP_data_file);
  assert(file.is_open());
  std::string val;
  uint64_t sp_id;
  while (file >> val) {  // file is formated as SP_id = obj_val
    if (val == "=") {
      continue;
    } else if (val[0] == 'S') {
      sp_id = std::stoi(val.erase(0, 3));
    } else if (val[0] >= '0' && val[0] <= '9') {
      const auto &res = obj_sp_pair.insert(std::make_pair(
          std::fabs(shared_info.subproblem_weight[sp_id] * std::stod(val)),
          sp_id));  // ascending
      assert(res.second);
    } else {
      const bool malformed_txt_file = false;
      assert(malformed_txt_file);
    }
  }
  file.close();
  assert(obj_sp_pair.size() == shared_info.num_subproblems);
  // pick the
  assert(!shared_info.retained_subproblem_ids.size());
  for (const auto pair : obj_sp_pair) {
    const auto &res = shared_info.retained_subproblem_ids.insert(pair.second);
    assert(res.second);
    if (shared_info.retained_subproblem_ids.size() >= num_retention) {
      break;
    }
  }
  assert(shared_info.retained_subproblem_ids.size() == num_retention);
}

void GetArtificialSubproblemWeights(const SharedInfo &shared_info,
                                    MasterSolverInfo &solver_info_) {
  solver_info_.sp_weights_to_create_art_sp.resize(shared_info.num_subproblems);
  double weight_sum = IloSum(shared_info.subproblem_weight);
  for (const auto sp_id : shared_info.retained_subproblem_ids) {
    weight_sum -= shared_info.subproblem_weight[sp_id];
    solver_info_.sp_weights_to_create_art_sp[sp_id] = 0;
  }
  for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
    if (!shared_info.retained_subproblem_ids.count(sp_id)) {
      solver_info_.sp_weights_to_create_art_sp[sp_id] =
          shared_info.subproblem_weight[sp_id] / (0.0 + weight_sum);
    }
  }
}
#endif
