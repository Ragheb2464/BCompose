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

void CreateSubproblemModels(const Data &data,
                            const std::shared_ptr<spdlog::logger> console) {
  for (int s = 0; s < data.num_scenario; ++s) {
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
    { // Creating the SP variables and a copy of the master variables
      z_var = IloNumVarArray(env, data.arcs_info.size(), 0, 1, ILOINT);
      (model).add(z_var);
      x_var = IloNumVarArray2(env, data.arcs_info.size());
      for (int a = 0; a < data.arcs_info.size(); a++) {
        x_var[a] = IloNumVarArray(env, data.num_od, 0, IloInfinity);
        (model).add(x_var[a]);
      }
    }

    { //! WARNING: Copied variables MUST be named, starting with "z_"
      for (const auto &arc : data.arcs_info) {
        const int arc_id = arc.second.arc_id;
        z_var[arc_id].setName(("z_" + std::to_string(arc_id)).c_str());
      }
    }
    { // SP variables can be named as desired
      uint64_t t = 0;
      for (int a = 0; a < data.arcs_info.size(); a++) {
        for (int k = 0; k < data.num_od; ++k) {
          x_var[a][k].setName(("x_" + std::to_string(t++)).c_str());
        }
      }
    }

    { // Creating the objective func
      //! WARNING: do not multiply to the probability, as it should be done  in
      //! the master problem
      //! WARNING: The SP formulation must be equal to the determistic version
      //! oof the problem under scenario s, i.e., fTy +cTx
      IloExpr expr(env);
      for (const auto &arc : data.arcs_info) {
        const int arc_id = arc.second.arc_id;
        const double flow_cost = arc.second.flow_cost;

        const double fixed_cost = arc.second.fixed_cost;
        expr += fixed_cost * z_var[arc_id];

        for (int k = 0; k < data.num_od; k++) {
          expr += flow_cost * x_var[arc_id][k];
        }
      }
      objective.setExpr(expr);
      (model).add(objective);
      expr.end();
    }

    { // Flow conservation constraaints
      for (const auto &od : data.od_pair) {
        const uint64_t k = od.first;
        const uint64_t origin_node_id = od.second.first;
        const uint64_t destination_node_id = od.second.second;
        assert(origin_node_id != destination_node_id);
        for (const auto &node : data.nodes_info) {
          const int node_id = node.first;
          const bool is_origin = true ? node_id == origin_node_id : false;
          const bool is_destination =
              true ? node_id == destination_node_id : false;

          IloExpr expr_exiting_edges(env);
          IloExpr expr_entering_edges(env);

          for (const auto exiting_edges : node.second.exiting_edges) {
            const int arc_id =
                data.arcs_info.at(std::make_pair(node_id, exiting_edges))
                    .arc_id;
            expr_exiting_edges += x_var[arc_id][k];
          }
          for (const auto entering_edges : node.second.entering_edges) {
            const int arc_id =
                data.arcs_info.at(std::make_pair(entering_edges, node_id))
                    .arc_id;
            expr_entering_edges += x_var[arc_id][k];
          }
          if (is_origin) {
            constraints.add(IloRange(env, std::fabs(data.demands[s][k]),
                                     expr_exiting_edges,
                                     std::fabs(data.demands[s][k])));
          } else if (is_destination) {
            constraints.add(IloRange(env, std::fabs(data.demands[s][k]),
                                     expr_entering_edges,
                                     std::fabs(data.demands[s][k])));
          } else if (!is_destination && !is_origin) {
            constraints.add(
                IloRange(env, 0, expr_exiting_edges - expr_entering_edges, 0));
          } else {
            assert(false);
          }
          expr_exiting_edges.end();
          expr_entering_edges.end();
        }
      }
    }

    { // Capcity constraints
      for (const auto &arc : data.arcs_info) {
        const int arc_id = arc.second.arc_id;
        const double capacity = arc.second.capacity;
        assert(capacity >= 0);
        IloExpr expr(env);
        for (int k = 0; k < data.num_od; k++) {
          expr += x_var[arc_id][k];
        }
        expr -= capacity * z_var[arc_id];
        constraints.add(IloRange(env, -IloInfinity, expr, 0));
        expr.end();
      }
    }

    { // Strong inequalities
      if (false) {
        for (const auto &arc : data.arcs_info) {
          const int arc_id = arc.second.arc_id;
          for (int k = 0; k < data.num_od; k++) {
            constraints.add(
                IloRange(env, -IloInfinity,
                         x_var[arc_id][k] -
                             std::fabs(data.demands[s][k]) * z_var[arc_id],
                         0));
          }
        }
      }
    }

    { // naming constraints
      // WARNING! we assume constraint 1 in SP_? corresponds to constraint 1 in
      // SP_i ... i.e., there is one-to-one mapping
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
#endif