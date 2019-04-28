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

void SolveExtForm(const std::shared_ptr<Data> data,
                  const std::shared_ptr<spdlog::logger> console) {
  const int n_sc = 456;
  IloEnv env;
  IloModel model;
  IloCplex cplex;
  IloObjective obj;
  IloNumVarArray y;
  IloNumVarArray2 piVar;
  IloRangeArray con;
  model = IloModel(env);
  cplex = IloCplex(model);
  con = IloRangeArray(env);
  obj = IloMinimize(env);
  //
  y = IloNumVarArray(env, data->getSizeD(), 0, 1, ILOINT);
  for (size_t i = 0; i < data->getSizeD(); i++) {
    y[i].setName(("y_" + std::to_string(i)).c_str());
  }
  (model).add(y);
  piVar = IloNumVarArray2(env, data->getN_nodes2());
  for (int i = 0; i < data->getN_nodes2(); i++) {
    piVar[i] = IloNumVarArray(env, n_sc, 0, 1e10);
    model.add(piVar[i]);
  }
  // obj
  IloExpr expr(env);
  for (int s = 0; s < n_sc; s++)
    expr += data->getP(s) * piVar[data->getOrigin(s)][s];
  obj.setExpr(expr);
  (model).add(obj);
  expr.end();
  for (int s = 0; s < n_sc; s++) {
    (con).add(IloRange(env, 1, piVar[data->getDestinations(s)][s], 1));
  }

  for (int s = 0; s < n_sc; s++) {
    for (int a = 0; a < data->getSizeD(); a++) {
      (con).add(IloRange(env, 0,
                         piVar[data->getTailD(a)][s] -
                             data->getQ(a) * piVar[data->getHeadD(a)][s],
                         IloInfinity));
    }
  }
  for (int s = 0; s < n_sc; s++) {
    for (int a = 0; a < data->getSizeAD(); a++) {
      (con).add(IloRange(env, 0,
                         piVar[data->getTailAD(a)][s] -
                             data->getR(a) * piVar[data->getHeadAD(a)][s],
                         IloInfinity));
    }
  }
  for (int s = 0; s < n_sc; s++) {
    for (int a = 0; a < data->getSizeD(); a++) {
      (con).add(IloRange(env, 0,
                         (data->getRD(a) - data->getQ(a)) *
                                 data->getPhi(s, data->getHeadD(a)) * y[a] +
                             piVar[data->getTailD(a)][s] -
                             data->getRD(a) * piVar[data->getHeadD(a)][s],
                         IloInfinity));
    }
  }

  for (int s = 0; s < n_sc; s++) {
    for (int i = 0; i < data->getN_nodes2(); i++) {
      if (data->getPhi(0, i) <= 0) {
        model.add(IloRange(env, 0, piVar[i][s], 0));
      }
    }
  }

  IloExpr expr1(env);
  for (IloInt a = 0; a < data->getSizeD(); a++) {
    expr1 += data->getC(a) * y[a];
  }
  // std::cout << data->getB() << '\n';
  model.add(IloRange(env, -IloInfinity, expr1, data->getB()));
  expr1.end();

  model.add(con);
  // cplex.exportModel("f.lp");
  {  // Make sure that the exported SP is feasible, otherwise the problem is
    // infeasible
    // cplex.setOut(env.getNullStream());
    // this is only to faster check the model
    IloNum start = cplex.getCplexTime();
    // cplex.setParam(IloCplex::ClockType, 2);
    // cplex.setParam(IloCplex::Param::TimeLimit, 36000);
    // cplex.setParam(IloCplex::Param::Threads, 1);
    // cplex.setParam(IloCplex::Param::Preprocessing::Relax, 0);
    // cplex.setParam(IloCplex::Param::Preprocessing::Linear, 0);
    // cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 0);
    // cplex.setParam(IloCplex::Param::MIP::Strategy::Search, 1);
    cplex.setParam(IloCplex::Param::Benders::Strategy, 3);
    // cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1.0);
    if (!cplex.solve()) {
      console->error(" Extenssive form is infeasible!");
      exit(0);
    }
    console->info("-obj= " + std::to_string(cplex.getObjValue()));
    console->info("-Time= " + std::to_string(cplex.getCplexTime() - start));
    console->info("-Gap= " + std::to_string(cplex.getMIPRelativeGap()));
    console->info(" ");
  }
}
#endif
