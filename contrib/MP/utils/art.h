#ifndef CONTRIB_MP_UTILES_ART_H
#define CONTRIB_MP_UTILES_ART_H

#include <limits>

#include "../../shared_info/structures.h"
#include "../structures.h"

void PrintWarning(const uint64_t sp_id_1, const uint64_t sp_id_2) {
  std::cout << " Subproblems SP_" << sp_id_1 << " and SP_" << sp_id_2
            << " are different!";
  std::cout << " WARNING, Turnning off the articulation ..." << std::endl;
}

uint64_t PickFirstSP(const SharedInfo &shared_info) {
  uint64_t picked_sp_id = std::numeric_limits<uint64_t>::max();
  for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
    if (!shared_info.retained_subproblem_ids.count(sp_id)) {
      picked_sp_id = sp_id;
      break;
    }
  }
  if (picked_sp_id == std::numeric_limits<uint64_t>::max()) {
    std::cout << "Turn off subproblem creation strategy" << std::endl;
    exit(0);
  }
  return picked_sp_id;
}

uint64_t CorrectObj(const IloEnv &env, IloObjective &objective,
                    const double weight) {
  uint64_t num_mp_vars = 0;
  IloExpr expr(env);
  for (IloExpr::LinearIterator it =
           IloExpr(objective.getExpr()).getLinearIterator();
       it.ok(); ++it) {
    const std::string var_name = it.getVar().getName();
    if (var_name.find("z_") != std::string::npos) {
      ++num_mp_vars;
      continue;
    }
    expr += weight * it.getCoef() * it.getVar();
  }
  objective.setExpr(expr);
  expr.end();
  return num_mp_vars;
}

void CorrectCons(IloRangeArray &constraints, const double weight) {
  for (IloInt con_id = 0; con_id < constraints.getSize(); ++con_id) {
    const std::string con_name = constraints[con_id].getName();
    assert(con_name.find("Reg_") != std::string::npos);
    IloExpr::LinearIterator it;
    for (it = IloExpr(constraints[con_id].getExpr()).getLinearIterator();
         it.ok(); ++it) {
      constraints[con_id].setLinearCoef(it.getVar(), weight * it.getCoef());
    }
  }
}

void CorrectRHS(IloRangeArray &constraints, const double weight) {
  for (IloInt con_id = 0; con_id < constraints.getSize(); ++con_id) {
    const IloNum lb = constraints[con_id].getLB();
    const IloNum ub = constraints[con_id].getUB();
    const auto set_lb = lb > -IloInfinity ? weight * lb : -IloInfinity;
    const auto set_ub = ub < IloInfinity ? weight * ub : IloInfinity;
    constraints[con_id].setBounds(set_lb, set_ub);
  }
}

bool UpdateObjCoeffs(const IloEnv &env, IloObjective &objective,
                     const IloObjective &aux_objective,
                     const std::unordered_map<std::string, IloNumVar> &var_map,
                     const double weight) {
  uint64_t num_mp_var = 0;
  IloExpr expr(env);
  IloExpr::LinearIterator it;
  for (it = IloExpr(aux_objective.getExpr()).getLinearIterator(); it.ok();
       ++it) {
    const std::string var_name = it.getVar().getName();
    if (var_name.find("z_") != std::string::npos) {
      ++num_mp_var;
      continue;
    }
    if (var_map.count(var_name)) {
      expr += it.getCoef() * var_map.at(var_name);
    } else {
      return false;
    }
  }
  objective.setExpr(objective.getExpr() + weight * expr);
  expr.end();
  // assert(num_mp_var == master_var_map.size());
  return true;
}

void UpdateConCoeffs(
    IloRangeArray &constraints, const IloRangeArray &aux_constraints,
    const std::unordered_map<std::string, IloNumVar> &var_map,
    const std::unordered_map<std::string, IloNumVar> &master_var_map,
    const double weight) {
  assert(constraints.getSize() == aux_constraints.getSize());
  for (IloInt con_id = 0; con_id < aux_constraints.getSize(); ++con_id) {
    // assert(constraints[con_id].getSize() ==
    // aux_constraints[con_id].getSize());
    const std::string con_name = aux_constraints[con_id].getName();
    assert(con_name.find("Reg_") != std::string::npos);
    IloExpr::LinearIterator it;
    IloExpr expr = constraints[con_id].getExpr();
    for (it = IloExpr(aux_constraints[con_id].getExpr()).getLinearIterator();
         it.ok(); ++it) {
      const std::string var_name = it.getVar().getName();
      if (var_map.count(var_name)) {
        expr += weight * it.getCoef() * var_map.at(var_name);
      } else if (master_var_map.count(var_name)) {
        expr += weight * it.getCoef() * master_var_map.at(var_name);
      } else {
        assert(false);
      }
    }
    constraints[con_id].setExpr(expr);
    expr.end();
  }
}

void UpdateRHS(IloRangeArray &constraints, const IloRangeArray &aux_constraints,
               const double weight) {
  for (IloInt con_id = 0; con_id < aux_constraints.getSize(); ++con_id) {
    const IloNum lb =
        constraints[con_id].getLB() + weight * aux_constraints[con_id].getLB();
    const IloNum ub =
        constraints[con_id].getUB() + weight * aux_constraints[con_id].getUB();
    const auto set_lb = lb > -IloInfinity ? lb : -IloInfinity;
    const auto set_ub = ub < IloInfinity ? ub : IloInfinity;
    constraints[con_id].setBounds(set_lb, set_ub);
  }
}

void AddArtSPToMP(const std::string &model_directory, SharedInfo &shared_info,
                  const MasterSolverInfo &solver_info_, MasterModel &mp_model) {
  assert(shared_info.subproblem_data.size() == shared_info.num_subproblems);
  //! the way it works is to load a SP and modify all its coeffs so that we
  //! get the art SP... we have the strong considtion that the formulation for
  //! every SP must be the same
  //! first chose a non-retained SP to be loaded
  if (Settings::GlobalScenarios::num_creation != 1) {
    printf("%s\n", "Wrong art settings!");
    exit(0);
  }
  uint64_t picked_sp_id = PickFirstSP(shared_info);
  std::unordered_map<std::string, IloNumVar> var_map;  // var_name->var
  std::unordered_map<std::string, IloNumVar> master_var_map;

  //! load the chosen SP
  IloEnv env = mp_model.env;
  IloModel model = IloModel(env);
  IloCplex cplex = IloCplex(model);
  IloObjective objective = IloObjective(env);
  IloRangeArray constraints = IloRangeArray(env);
  IloNumVarArray variables = IloNumVarArray(env);
  const std::string model_dir =
      model_directory + "/SP_" + std::to_string(picked_sp_id) + ".sav";
  cplex.importModel(model, model_dir.c_str(), objective, variables,
                    constraints);

  {  // create the var map
    for (size_t var_id = 0; var_id < variables.getSize(); ++var_id) {
      const std::string var_name = variables[var_id].getName();
      if (var_name.find("z_") == std::string::npos) {
        const auto &res = var_map.emplace(var_name, variables[var_id]);
        assert(res.second);
      } else {
        const auto &res = master_var_map.emplace(var_name, variables[var_id]);
        assert(res.second);
      }
    }
  }
  {  // correct obj of picked_sp_id
    const uint64_t num_mp_vars =
        CorrectObj(mp_model.env, objective,
                   solver_info_.sp_weights_to_create_art_sp[picked_sp_id]);
    if (num_mp_vars && num_mp_vars != master_var_map.size()) {
      std::cout << "ERROR: num copied vars are not the same in the SPs"
                << std::endl;
      exit(0);
    }
  }
  {  // correct cons
    CorrectCons(constraints,
                solver_info_.sp_weights_to_create_art_sp[picked_sp_id]);
  }

  {  // correcct rhs
    CorrectRHS(constraints,
               solver_info_.sp_weights_to_create_art_sp[picked_sp_id]);
  }
  // getting convex combination of the uncertain paramters
  // NOTE: we have the data for all constraints except for the NACs and CPLEX
  // cuts
  for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
    if (!shared_info.retained_subproblem_ids.count(sp_id) ||
        sp_id == picked_sp_id) {
      continue;
    }
    {  // loading the sp
      IloEnv aux_env = mp_model.env;
      IloModel aux_model = IloModel(aux_env);
      IloCplex aux_cplex = IloCplex(aux_model);
      IloObjective aux_objective = IloObjective(aux_env);
      IloRangeArray aux_constraints = IloRangeArray(env);
      IloNumVarArray aux_variables = IloNumVarArray(aux_env);
      const std::string aux_model_dir =
          model_directory + "/SP_" + std::to_string(sp_id) + ".sav";
      aux_cplex.importModel(aux_model, aux_model_dir.c_str(), aux_objective,
                            aux_variables, aux_constraints);

      const double weight = solver_info_.sp_weights_to_create_art_sp[sp_id];
      bool status = UpdateObjCoeffs(mp_model.env, objective, aux_objective,
                                    var_map, weight);
      if (!status) {
        PrintWarning(sp_id, picked_sp_id);
        exit(911);
      }
      UpdateConCoeffs(constraints, aux_constraints, var_map, master_var_map,
                      weight);
      UpdateRHS(constraints, aux_constraints, weight);
    }
  }
  // model.add(constraints);
  {  // adding art SP to mp
    mp_model.model.add(variables);
    {  // obj
      double weight_sum = 0;
      IloExpr expr(env);
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          assert(!solver_info_.sp_weights_to_create_art_sp[sp_id]);
          continue;
        }
        const double weight = solver_info_.sp_weights_to_create_art_sp[sp_id];
        weight_sum += weight;
        expr += weight * mp_model.recourse_variables[sp_id];
      }
      assert(std::round(weight_sum) == 1);
      mp_model.model.add(
          IloRange(mp_model.env, -IloInfinity, objective.getExpr() - expr, 0));
      expr.end();
    }
    {  // enforcing z=y
      uint64_t var_id;
      uint64_t NAC_count = 0;
      for (uint64_t i = 0; i < variables.getSize(); ++i) {
        std::string var_name = variables[i].getName();
        std::size_t found = var_name.find("z_");
        if (found != std::string::npos) {
          var_id = std::stoi(var_name.erase(found, 2));
          assert(var_id < mp_model.master_variables.getSize());
          constraints.add(IloRange(
              env, 0, variables[i] - mp_model.master_variables[var_id], 0));
          ++NAC_count;
        }
      }
      assert(NAC_count == mp_model.master_variables.getSize());
    }
  }
  // cplex.exportModel("art.lp");
  {  // constraints
    mp_model.model.add(constraints);
  }
  {  // relax z vars if they are int
    if (cplex.isMIP()) {
      mp_model.model.add(IloConversion(env, variables, ILOFLOAT));
    }
  }
  // mp_model.cplex.exportModel("MP.lp");
  // exit(0);
  // clean up
  // shared_info.subproblem_data.clear();
}

#endif
