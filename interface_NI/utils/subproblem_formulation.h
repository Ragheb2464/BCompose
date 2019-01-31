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

void CreateSubproblemModels(const std::shared_ptr<Data> data,
                            const std::shared_ptr<spdlog::logger> console) {
  for (int s = 0; s < data->getN_sc(); ++s) {
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
    //! will be truned  off automatically
    { // Creating the SP variables and a copy of the master variables
      z_var = IloNumVarArray(env, data->getSizeD(), 0, 1, ILOINT);
      (model).add(z_var);
      x_var = IloNumVarArray(env, data->getN_nodes2(), 0, IloInfinity);
      (model).add(x_var);
    }

    { //! WARNING: Copied variables MUST be named, starting with "z_"
      for (size_t i = 0; i < data->getSizeD(); i++) {
        {
          const int arc_id = i;
          z_var[arc_id].setName(("z_" + std::to_string(arc_id)).c_str());
        }
      }

      { // Creating the objective func
        //! WARNING: do not multiply to the probability, as it should be done in
        //! the master problem
        //! WARNING: The SP formulation must be equal to the determistic version
        //! oof the problem under scenario s, i.e., fTy +cTx
        objective.setExpr(x_var[data->getOrigin(s)]);
        (model).add(objective);
      }

      { // Flow conservation constraaints
        (constraints).add(IloRange(env, 1, x_var[data->getDestinations(s)], 1));
        for (int a = 0; a < data->getSizeD(); a++)
          (constraints)
              .add(IloRange(env, 0,
                            x_var[data->getTailD(a)] -
                                data->getQ(a) * x_var[data->getHeadD(a)],
                            IloInfinity));
        for (int a = 0; a < data->getSizeAD(); a++)
          (constraints)
              .add(IloRange(env, 0,
                            x_var[data->getTailAD(a)] -
                                data->getR(a) * x_var[data->getHeadAD(a)],
                            IloInfinity));
        //
        for (int a = 0; a < data->getSizeD(); a++)
          (constraints)
              .add(IloRange(env, 0,
                            (data->getRD(a) - data->getQ(a)) *
                                    data->getPhi(s, data->getHeadD(a)) *
                                    z_var[a] +
                                x_var[data->getTailD(a)] -
                                data->getRD(a) * x_var[data->getHeadD(a)],
                            IloInfinity));
        //
        for (int i = 0; i < data->getN_nodes2(); i++)
          if (data->getPhi(0, i) <= 0)
            constraints.add(IloRange(env, 0, x_var[i], 0));
        //
        IloExpr expr1(env);
        for (IloInt a = 0; a < data->getSizeD(); a++)
          expr1 += data->getC(a) * z_var[a];
        constraints.add(IloRange(env, 0, expr1, data->getB()));
        expr1.end();
      }

      { // naming constraints
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
        const std::string export_dir = "../models/";
        cplex.exportModel(
            (export_dir + "SP_" + std::to_string(s) + ".sav").c_str());
      }

      { // Make sure that the exported SP is feasible, otherwise the problem is
        // infeasible
        cplex.setOut(env.getNullStream());
        // this is only to faster check the model
        cplex.setParam(IloCplex::Param::MIP::Tolerances::MIPGap, 1.0);
        cplex.setParam(IloCplex::Param::Threads, 7);
        if (!cplex.solve()) {
          console->error(" Subproblem is infeasible!");
          exit(0);
        }
      }
    }
  }
}
#endif
