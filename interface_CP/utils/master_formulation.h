#ifndef INTERFACE_CP_UTILS_MASTER
#define INTERFACE_CP_UTILS_MASTER

/*!
  This is where the user writtes initial master formulation.
    WARNING: Numbering of master variables should match the copied ones in SPs
    NOTE: here is an example for the !
*/

typedef IloArray<IloNumVarArray> IloNumVarArray2;
typedef IloArray<IloNumVarArray2> IloNumVarArray3;
typedef IloArray<IloNumVarArray3> IloNumVarArray4;

void CreateMasterModel(const std::shared_ptr<spdlog::logger> console) {
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
  // WARNING initial MP is an LP
  y_var = IloNumVarArray(env, I, 0, 1);
  // naming
  for (const auto& it : fixed_cost) {
    const auto var_id = it.first;
    y_var[var_id].setName(("y_" + std::to_string(var_id))
                              .c_str());  // I have named the variables for
                                          // consistancy when readdin the data
  }
  theta = IloNumVarArray(env, 1, -1e16, IloInfinity);
  theta[0].setName(("theta_" + std::to_string(0)).c_str());
  model.add(y_var);
  model.add(theta);

  {  // Creating the objective func
    IloExpr expr(env);
    // for (size_t i = 0; i < I; ++i) {
    //   const auto var_id = i;
    //   const auto fixed_cost = f[i];
    //   expr += fixed_cost * y_var[var_id];
    // }
    expr += theta[0];
    objective.setExpr(expr);
    (model).add(objective);
    expr.end();
  }

  IloExpr expr_f(env);
  for (const auto& it : fixed_cost) {
    expr_f += it.second * y_var[it.first];
  }
  model.add(IloRange(env, -IloInfinity, expr_f, B));
  expr_f.end();
  // console->info("  -VI");
  // {
  //   IloExpr expr(env);
  //   for (size_t j = 0; j < J; ++j) {
  //     for (size_t i = 0; i < set_J_i[j].size(); ++i) {
  //       const auto var_id = set_J_i[j][i];
  //       expr += d[j] * y_var[var_id];
  //     }
  //   }
  //   model.add(IloRange(env, std::ceil(D), expr, IloInfinity));
  // }

  console->info("    Exporting formulation for master problem...");
  cplex.setWarning(env.getNullStream());
  // WARNING: Exported model must be named as MP.sav
  cplex.exportModel("../models/MP.sav");
  console->info("    Done!");
}
#endif
