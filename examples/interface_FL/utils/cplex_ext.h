#ifndef CONTRIB_INTERFACE_SP_H
#define CONTRIB_INTERFACE_SP_H

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
typedef IloArray<IloNumVarArray2> IloNumVarArray3;

void CreateSubproblemModels(const std::shared_ptr<Data_S> data,
                            const std::shared_ptr<spdlog::logger> console) {
  double max_demand = 0;
  double expected_obj = 0;
  for (int s = 0; s < data->getN_sc(); s++) {
    double aux = 0;
    for (int j = 0; j < data->getNumCustomers(); j++) {
      aux += std::fabs(data->getD(s, j));
    }
    if (aux > max_demand) {
      max_demand = aux;
    }
  }

  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective objective;
  IloNumVarArray3 x_var;
  IloNumVarArray z_var;
  IloRangeArray constraints;

  model = IloModel(env);
  cplex = IloCplex(model);
  constraints = IloRangeArray(env);
  objective = IloMinimize(env);

  {
    z_var = IloNumVarArray(env, data->getNumFacilityNode(), 0, 1, ILOINT);
    (model).add(z_var);
    //
    x_var = IloNumVarArray3(env, data->getNumFacilityNode());
    for (int i = 0; i < data->getNumFacilityNode(); i++) {
      x_var[i] = IloNumVarArray2(env, data->getNumCustomers());
      for (int j = 0; j < data->getNumCustomers(); ++j) {
        x_var[i][j] = IloNumVarArray(env, data->getN_sc(), 0, IloInfinity);
        model.add(x_var[i][j]);
      }
    }
  }

  {
    IloExpr expr(env);
    for (int i = 0; i < data->getNumFacilityNode(); i++) {
      expr += data->getF(i) * z_var[i];
      assert(data->getF(i) >= 0);
      for (int s = 0; s < data->getN_sc(); s++) {
        for (int j = 0; j < data->getNumCustomers(); j++) {
          expr +=
              std::fabs(data->getP(s)) * data->getC(i, j, s) * x_var[i][j][s];
          assert(data->getC(i, j, s) >= 0);
        }
      }
    }
    objective.setExpr(expr);
    (model).add(objective);
  }

  {
    for (int s = 0; s < data->getN_sc(); s++) {
      // assignment
      for (int j = 0; j < data->getNumCustomers(); j++) {
        IloExpr expr(env);
        for (int i = 0; i < data->getNumFacilityNode(); i++) {
          expr += x_var[i][j][s];
        }
        constraints.add(
            IloRange(env, std::fabs(data->getD(s, j)), expr, IloInfinity));
        expr.end();
      }
      // capacity
      for (int i = 0; i < data->getNumFacilityNode(); i++) {
        IloExpr expr(env);
        for (int j = 0; j < data->getNumCustomers(); j++) {
          expr += x_var[i][j][s];
        }
        expr -= std::fabs(data->getU(i, s)) * z_var[i];
        constraints.add(IloRange(env, -IloInfinity, expr, 0));
        expr.end();
      }
    }
  }

  // SI cuts
  if (true) {
    for (int s = 0; s < data->getN_sc(); s++) {
      for (int j = 0; j < data->getNumCustomers(); j++) {
        for (int i = 0; i < data->getNumFacilityNode(); i++) {
          constraints.add(IloRange(
              env, -IloInfinity,
              x_var[i][j][s] - std::fabs(data->getD(s, j)) * z_var[i], 0));
        }
      }
    }
  }

  model.add(constraints);

  {
    IloNum start = cplex.getCplexTime();
    // cplex.setOut(env.getNullStream());
    cplex.setParam(IloCplex::Param::Threads, 8);
    cplex.setParam(IloCplex::Param::Benders::Strategy, 3);
    if (!cplex.solve()) {
      console->error(" Subproblem is infeasible!");
      exit(0);
    }
    console->info("-obj=        " + std::to_string(cplex.getObjValue()));
    console->info("-Time=       " +
                  std::to_string(cplex.getCplexTime() - start));
    console->info("-Gap=        " + std::to_string(cplex.getMIPRelativeGap()));
    console->info(" ");
  }
}
#endif
