#ifndef INTERFACE_FL_UTILS_MASTER_PROBLEM_H
#define INTERFACE_FL_UTILS_MASTER_PROBLEM_H

/*!
  This is where the user writtes initial master formulation.
    WARNING: Numbering of master variables should match the copied ones in SPs
    NOTE: here is an example for the SND!
*/

typedef IloArray<IloNumVarArray> IloNumVarArray2;
typedef IloArray<IloNumVarArray2> IloNumVarArray3;
typedef IloArray<IloNumVarArray3> IloNumVarArray4;

void CreateMasterModel(const std::shared_ptr<Data_S> data,
                       const std::shared_ptr<spdlog::logger> console) {
  //! WARNING: Name the variables
  // std::cout << "/* message */" << '\n' << std::flush;
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
  y_var = IloNumVarArray(env, data->getNumFacilityNode(), 0, 1);
  // naming
  for (size_t i = 0; i < data->getNumFacilityNode(); i++) {
    const int arc_id = i;
    y_var[arc_id].setName(("y_" + std::to_string(arc_id)).c_str());
  }
  theta = IloNumVarArray(env, data->getN_sc(), 0, IloInfinity);
  for (int sp_id = 0; sp_id < data->getN_sc(); sp_id++) {  // naming
    //! WARNING: recourse variables must be name with "theta_id"
    theta[sp_id].setName(("theta_" + std::to_string(sp_id)).c_str());
  }
  model.add(y_var);
  model.add(theta);

  // console->info("    Adding master objective...");
  {
    IloExpr expr(env);
    for (int s = 0; s < data->getN_sc(); s++) {
      expr += data->getP(s) * theta[s];
    }
    for (IloInt a = 0; a < data->getNumFacilityNode(); a++) {
      expr += data->getF(a) * y_var[a];
      // std::cout << data->getF(a) << '\n';
    }
    objective.setExpr(expr);
    (model).add(objective);
    expr.end();
  }
  // console->info("    Adding constraints...");
  {
    if (true) {
      float max_demand = 0;
      int sc_id;
      for (int s = 0; s < data->getN_sc(); s++) {
        float aux = 0;
        for (int j = 0; j < data->getNumCustomers(); j++) {
          aux += data->getD(s, j);
        }
        if (aux > max_demand) {
          max_demand = aux;
          sc_id = s;
        }
      }
      // std::cout << "max_demand is " << max_demand << " for scenario " <<
      // sc_id
      //           << '\n';
      for (int s = 0; s < data->getN_sc(); s++) {
        IloExpr expr(env);
        for (IloInt a = 0; a < data->getNumFacilityNode(); a++) {
          expr += data->getU(a, s) * y_var[a];
        }
        model.add(IloRange(env, ceil(max_demand), expr, IloInfinity));
        expr.end();
      }
    }
  }

  // console->info("    Exporting formulation for master problem...");
  cplex.setWarning(env.getNullStream());
  // WARNING: Exported model must be named as MP.sav
  cplex.exportModel("../../models/MP.sav");
  console->info("    Done!");
}
#endif
