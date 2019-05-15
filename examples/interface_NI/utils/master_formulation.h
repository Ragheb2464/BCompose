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

void CreateMasterModel(const std::shared_ptr<Data> data,
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
  y_var = IloNumVarArray(env, data->getSizeD(), 0, 1);
  // naming
  for (size_t i = 0; i < data->getSizeD(); i++) {
    const int arc_id = i;
    y_var[arc_id].setName(("y_" + std::to_string(arc_id)).c_str());
  }
  theta = IloNumVarArray(env, data->getN_sc(), -1e11, IloInfinity);
  for (int sp_id = 0; sp_id < data->getN_sc(); sp_id++) {  // naming
    //! WARNING: recourse variables must be name with "theta_id"
    theta[sp_id].setName(("theta_" + std::to_string(sp_id)).c_str());
  }
  model.add(y_var);
  model.add(theta);

  // console->info("    Adding master objective...");
  {
    IloExpr expr(env);
    for (int s = 0; s < data->getN_sc(); s++) expr += data->getP(s) * theta[s];
    objective.setExpr(expr);
    (model).add(objective);
    expr.end();
  }

  // console->info("    Adding master constraints (if any)...");
  IloExpr expr1(env);
  for (IloInt a = 0; a < data->getSizeD(); a++)
    expr1 += data->getC(a) * y_var[a];
  model.add(IloRange(env, 0, expr1, data->getB()));
  expr1.end();

  // console->info("    Exporting formulation for master problem...");
  cplex.setWarning(env.getNullStream());
  // WARNING: Exported model must be named as MP.sav
  cplex.exportModel("../../models/MP.sav");
  // console->info("    Done!");
}
#endif
