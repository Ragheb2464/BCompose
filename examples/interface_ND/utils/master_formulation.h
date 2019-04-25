#ifndef CONTRIB_INTERFACE_MP_H
#define CONTRIB_INTERFACE_MP_H

/*!
  This is where the user writtes initial master formulation.
    WARNING: Numbering of master variables should match the copied ones in SPs
    NOTE: here is an example for the SND!
*/

typedef IloArray<IloNumVarArray> IloNumVarArray2;
typedef IloArray<IloNumVarArray2> IloNumVarArray3;
typedef IloArray<IloNumVarArray3> IloNumVarArray4;

void CreateMasterModel(const Data &data,
                       const std::shared_ptr<spdlog::logger> console) {
  //! WARNING: Name the variables
  // console->info("    Creating master objects...");
  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective objective;
  IloNumVarArray y_var, theta;
  IloRangeArray constraints;
  model = IloModel(env);
  cplex = IloCplex(model);
  objective = IloMinimize(env);
  constraints = IloRangeArray(env);

  // console->info("    Creating master variables...");
  y_var = IloNumVarArray(env, data.num_arcs, 0, 1);
  // naming
  for (const auto &arc : data.arcs_info) {
    const int arc_id = arc.second.arc_id;
    y_var[arc_id].setName(("y_" + std::to_string(arc_id)).c_str());
  }
  theta = IloNumVarArray(env, data.num_scenario, 0, IloInfinity);
  for (int sp_id = 0; sp_id < data.num_scenario; sp_id++) {  // naming
    //! WARNING: recourse variables must be name with "theta_id"
    theta[sp_id].setName(("theta_" + std::to_string(sp_id)).c_str());
  }
  model.add(y_var);
  model.add(theta);

  // console->info("    Adding master objective...");
  {
    IloExpr expr(env);
    for (const auto &it : data.arcs_info) {
      const int arc_id = it.second.arc_id;
      const double fixed_cost = it.second.fixed_cost;
      assert(arc_id < data.arcs_info.size());
      expr += fixed_cost * y_var[arc_id];
    }
    for (int sp_id = 0; sp_id < data.num_scenario; sp_id++) {
      expr += data.scenario_probability[sp_id] * theta[sp_id];
    }

    objective.setExpr(expr);
    (model).add(objective);
    expr.end();
  }

  // console->info("    Adding master constraints (if any)...");
  //! None in this example

  console->info("    Exporting formulation for master problem...");
  cplex.setWarning(env.getNullStream());
  // WARNING: Exported model must be named as MP.sav
  cplex.exportModel("../../models/MP.sav");
  // console->info("    Done!");
}
#endif
