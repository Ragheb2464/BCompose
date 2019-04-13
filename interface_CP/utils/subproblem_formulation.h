#ifndef INTERFACE_CP_CP_UTILS_SUBPROBLEM
#define INTERFACE_CP_CP_UTILS_SUBPROBLEM

/** @file subproblem_formulation.h
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     9/23/2018
@brief    This part creates and exports the an MILP SP model for each scenario.
*/
/*!
  This is where the user writtes subproblem formulation.
    NOTE:The model should be the same as the determinstic problem
    NOTE: A model should be exported for each scenario!
    NOTE: here is an example for the SND!
    WARNING: Try to keep the subproblem formulations the same, otherwise some
              internal accelerations might get turned off
    WARNING: Constraints must be named as Reg_{number},
    WARNING: Problem must be minimization (convert it if it is not)
*/

typedef IloArray<IloNumVarArray> IloNumVarArray2;

void CreateSubproblemModels(const std::shared_ptr<spdlog::logger> console) {
  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective objective;
  IloNumVarArray x_var;
  IloNumVarArray z_var;
  IloRangeArray constraints;

  model = IloModel(env);
  cplex = IloCplex(model);
  constraints = IloRangeArray(env);
  objective = IloMinimize(env);

  //! WARNING: copied variables must be INTEGRAL, otherwise main accelerator
  //! will be truned  off
  {  // Creating the SP variables and a copy of the master variables
    z_var = IloNumVarArray(env, I, 0, 1, ILOINT);
    x_var = IloNumVarArray(env, J, 0, 1);
    (model).add(z_var);
    (model).add(x_var);
  }
  console->info("  -naming z_");
  {  //! WARNING: Copied variables MUST be named, starting with "z_"
    for (const auto& it : fixed_cost) {
      const auto var_id = it.first;
      z_var[var_id].setName(("z_" + std::to_string(var_id))
                                .c_str());  // I have named the variables for
                                            // consistancy when readdin the data
    }
  }
  {  // SP variables can be named as desired
     // for (size_t j = 0; j < J; ++j) {
     //   x_var[j].setName(("x_" + std::to_string(j)).c_str());
     // }
  }
  console->info("  -obj");
  {  // Creating the objective func
    //! WARNING: do not multiply to the probability, as it should be done  in
    //! the master problem
    //! WARNING: The SP formulation must be equal to the determistic version
    //! oof the problem under scenario s, i.e., fTy +cTx
    IloExpr expr(env);
    for (const auto& it : demand) {
      expr -= it.second * x_var[it.first];
    }
    objective.setExpr(expr);
    (model).add(objective);
    expr.end();
    // IloExpr expr(env);
    // for (size_t i = 0; i < I; ++i) {
    //   const auto var_id = i;
    //   const double fixed_cost = f[var_id];
    //   expr += fixed_cost * z_var[var_id];
    // }
    // objective.setExpr(expr);
    // (model).add(objective);
    // expr.end();
  }

  console->info("  -cons");
  {  // linking constraaints
     // IloExpr expr_d(env);
    double max_demand = 0;
    double lost_demand = 0;
    for (const auto& it : demand) {
      // expr_d += it.second * x_var[it.first];
      IloExpr expr(env);
      if (set_J_i.count(it.first)) {
        max_demand += it.second;
        for (const auto var_id : set_J_i.at(it.first)) {
          expr += z_var[var_id];
        }
      } else {
        lost_demand += it.second;
        // std::cout << "Customer without near facility site: " << it.first
        //           << '\n';
      }
      expr -= x_var[it.first];
      constraints.add(IloRange(env, 0, expr, IloInfinity));
      expr.end();
    }
    // constraints.add(IloRange(env, D, expr_d, IloInfinity));
    // expr_d.end();
    std::cout << max_demand << " is max obj and " << lost_demand
              << " is lost demand \n";
  }
  {
    IloExpr expr_f(env);
    for (const auto& it : fixed_cost) {
      expr_f += it.second * z_var[it.first];
    }
    constraints.add(IloRange(env, -IloInfinity, expr_f, B));
    expr_f.end();
  }
  {  // anti symmetry
     // if z_i and z_j has the same exact contribution, force z_i<=z_j
     // we know that their contribution is the same in the budget constraint
    // we need to check their contribution to coverage
    for (const auto& it : facility_to_covered_demand) {
      const uint64_t f_id = it.first;
      const double covered_demand = it.second;
      // std::cout << covered_demand << '\n';
      for (uint64_t i = f_id + 1; i < facility_to_covered_demand.size(); ++i) {
        if (covered_demand == facility_to_covered_demand.at(i)) {
          // constraints.add(
          //     IloRange(env, -IloInfinity, z_var[i] - z_var[f_id], 0));
          std::cout << "Equal facilities => " << f_id << "=" << i << '\n';
        }
      }
    }
  }
  // 3484664.000000
  // console->info("  -VI");
  // {
  //   IloExpr expr(env);
  //   for (size_t j = 0; j < J; ++j) {
  //     for (size_t i = 0; i < set_J_i[j].size(); ++i) {
  //       const auto var_id = set_J_i[j][i];
  //       expr += d[j] * z_var[var_id];
  //     }
  //   }
  //   constraints.add(IloRange(env, std::ceil(D), expr, IloInfinity));
  // }

  console->info("  -naming cons");
  {  // naming constraints
    // WARNING! we assume constraint 1 in SP_? corresponds to constraint 1 in
    // SP_i ... i.e., there is one-to-one mapping
    for (uint64_t i = 0; i < constraints.getSize(); ++i) {
      constraints[i].setName(("Reg_" + std::to_string(i)).c_str());
    }
  }
  model.add(constraints);
  console->info("  -solving it");
  {  // Make sure that the exported SP is feasible, otherwise the problem is
    // infeasible
    // cplex.setOut(env.getNullStream());
    // this is only to faster check the model
    // cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1.0);
    // cplex.setParam(IloCplex::Param::Preprocessing::Linear, 0);
    // cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 0);
    cplex.setParam(IloCplex::Param::Threads, 7);
    // cplex.setParam(IloCplex::Param::Benders::Strategy, 3);
    if (!cplex.solve()) {
      console->error(" Subproblem is infeasible!");
      exit(0);
    }
    console->error(" Obj val is " + std::to_string(cplex.getObjValue()));
  }

  console->info("    Exporting formulation for subproblem " +
                std::to_string(0) + "...");
  {
    cplex.setWarning(env.getNullStream());
    // WARNING: Exported model must be named as SP_{s}.sav
    // NOTE: Even if there is one subproblem, it must be named as SP_0.sav
    const std::string export_dir = "../models/";
    cplex.exportModel(
        (export_dir + "SP_" + std::to_string(0) + ".sav").c_str());
  }

  console->info("  -Done!");
}
#endif
