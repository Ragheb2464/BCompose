// Copyright 2018, Ragheb Rahmaniani
#ifndef INTERFACE_FL_UTILS_SUBPROBLEM_H
#define INTERFACE_FL_UTILS_SUBPROBLEM_H

#include <string>
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

  for (int s = 0; s < data->getN_sc(); ++s) {
    IloEnv env;
    IloModel model;
    IloCplex cplex;
    IloObjective objective;
    IloNumVarArray2 x_var;
    IloNumVarArray z_var;
    IloRangeArray constraints;

    model = IloModel(env);
    cplex = IloCplex(model);
    constraints = IloRangeArray(env);
    objective = IloMinimize(env);

    //! WARNING: copied variables must be INTEGRAL, otherwise main accelerator
    //! will be truned  off automatically
    {  // Creating the SP variables and a copy of the master variables
      z_var = IloNumVarArray(env, data->getNumFacilityNode(), 0, 1, ILOINT);
      (model).add(z_var);
      x_var = IloNumVarArray2(env, data->getNumFacilityNode());
      for (int i = 0; i < data->getNumFacilityNode(); i++) {
        x_var[i] = IloNumVarArray(env, data->getNumCustomers(), 0, IloInfinity);
        model.add(x_var[i]);
      }
    }

    {  //! WARNING: Copied variables MUST be named, starting with "z_"
      for (size_t i = 0; i < data->getNumFacilityNode(); i++) {
        {
          const int arc_id = i;
          z_var[arc_id].setName(("z_" + std::to_string(arc_id)).c_str());
        }
      }

      {  // Creating the objective func
        //! WARNING: do not multiply to the probability, as it should be done
        //! in the master problem WARNING: The SP formulation must be equal to
        //! the determistic version oof the problem under scenario s, i.e., fTy
        //! +cTx
        IloExpr expr(env);
        for (int i = 0; i < data->getNumFacilityNode(); i++) {
          expr += data->getF(i) * z_var[i];
          assert(data->getF(i) >= 0);
          for (int j = 0; j < data->getNumCustomers(); j++) {
            expr += data->getC(i, j, s) * x_var[i][j];
            assert(data->getC(i, j, s) >= 0);
          }
        }
        objective.setExpr(expr);
        (model).add(objective);
      }

      {
        for (int j = 0; j < data->getNumCustomers(); j++) {
          IloExpr expr(env);
          for (int i = 0; i < data->getNumFacilityNode(); i++) {
            expr += x_var[i][j];
          }
          constraints.add(
              IloRange(env, std::fabs(data->getD(s, j)), expr, IloInfinity));
          expr.end();
        }
        for (int i = 0; i < data->getNumFacilityNode(); i++) {
          IloExpr expr(env);
          for (int j = 0; j < data->getNumCustomers(); j++) {
            expr += x_var[i][j];
          }
          expr -= std::fabs(data->getU(i, s)) * z_var[i];
          constraints.add(IloRange(env, -IloInfinity, expr, 0));
          expr.end();
        }
        // When the complete recourse problem is solved
        if (true) {
          IloExpr expr(env);
          for (IloInt a = 0; a < data->getNumFacilityNode(); a++)
            expr += std::fabs(data->getU(a, s)) * z_var[a];
          model.add(IloRange(env, ceil(max_demand), expr, IloInfinity));
          expr.end();
        }
        // SI cuts
        if (true) {
          for (int j = 0; j < data->getNumCustomers(); j++) {
            for (int i = 0; i < data->getNumFacilityNode(); i++) {
              constraints.add(IloRange(
                  env, -IloInfinity,
                  x_var[i][j] - std::fabs(data->getD(s, j)) * z_var[i], 0));
            }
          }
        }
      }

      {  // naming constraints
        // WARNING! we assume constraint 1 in SP_? corresponds to constraint 1
        // in SP_i ... i.e., there is one-to-one mapping
        for (uint64_t i = 0; i < constraints.getSize(); ++i) {
          constraints[i].setName(("Reg_" + std::to_string(i)).c_str());
        }
      }
      model.add(constraints);

      // console->info("    Exporting formulation for subproblem " +
      //               std::to_string(s) + "...");
      {
        cplex.setWarning(env.getNullStream());
        // WARNING: Exported model must be named as SP_{s}.sav
        // NOTE: Even if there is one subproblem, it must be named as SP_0.sav
        const std::string export_dir = "../../models/";
        cplex.exportModel(
            (export_dir + "SP_" + std::to_string(s) + ".sav").c_str());
      }

      {  // Make sure that the exported SP is feasible, otherwise the problem
        // is
        // infeasible
        cplex.setOut(env.getNullStream());
        // this is only to faster check the model
        cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1.0);
        cplex.setParam(IloCplex::Param::Threads, 7);
        if (!cplex.solve()) {
          console->error(" Subproblem is infeasible!");
          exit(0);
        }
        expected_obj += cplex.getObjValue();
      }
    }
  }
  console->info("  -Expected Obj is=" +
                std::to_string(expected_obj / data->getN_sc()));
}
#endif  // INTERFACE_FL_UTILS_SUBPROBLEM_H
