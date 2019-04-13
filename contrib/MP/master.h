#ifndef CONTRIB_MP_MASTER_H
#define CONTRIB_MP_MASTER_H
/** @file master.h
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates the parallelization framework.
*/
#include "../SP/subproblem.h"
#include "../control/solver_settings.h"
#include "../heuristic/heuristic.h"
#include "../shared_info/structures.h"
#include "callbacks.h"
#include "loader.h"
#include "structures.h"
#include "utils/art.h"
#include "utils/partial_decomposition.h"
#include "utils/warm_start.h"

class Master {
public:
  Master() {}
  ~Master() {}

  void Initializer(const std::shared_ptr<spdlog::logger> console,
                   SharedInfo &shared_info,
                   const std::string current_directory) {
    SetSartTime();
    //! Load the model
    const bool loading_status =
        Loader(console, master_model_, shared_info, current_directory);
    if (!loading_status) {
      console->info("   Failed to load the MP.lp!");
      console->info("  Exiting with error. ");
      exit(0);
    }

    //! initialize the params
    shared_info.master_variables_value =
        IloNumArray(master_model_.env, shared_info.num_master_variables);
    shared_info.recourse_variables_value =
        IloNumArray(master_model_.env, shared_info.num_recourse_variables);
    shared_info.num_subproblems = shared_info.num_recourse_variables;
    //
    num_threads_ = 1;
    console->info("    Running master on " + std::to_string(num_threads_) +
                  " core(s).");
    master_model_.cplex.setParam(IloCplex::Param::Threads, num_threads_);
  }

  void FixSafeVariables(const std::shared_ptr<spdlog::logger> console,
                        SharedInfo &shared_info) {
    if (shared_info.fixed_master_variables.size() &&
        Settings::Heuristic::safe_fix) {
      for (const auto &it : shared_info.fixed_master_variables) {
        uint64_t var_id = it.first;
        double val = it.second;
        master_model_.master_variables[var_id].setBounds(val, val);
      }
      console->info(
          "   Aggressive presolve could permanently fix " +
          std::to_string(shared_info.fixed_master_variables.size()) +
          " master variables out of " +
          std::to_string(shared_info.master_variables_value.getSize()));
    }
  }

  void PartialDecompsoition(const std::string model_directory,
                            const std::shared_ptr<spdlog::logger> console,
                            SharedInfo &shared_info,
                            const std::string current_directory) {
    assert(shared_info.num_subproblems >=
           Settings::GlobalScenarios::num_retention);
    if (Settings::GlobalScenarios::num_creation &&
        (shared_info.num_subproblems == 1 ||
         Settings::GlobalScenarios::num_retention ==
             shared_info.num_subproblems)) {
      console->error(
          "   Having artificial subproblems does not make sense to me!");
      exit(0);
    }
    // TODO: Use other techniques to pick global SPs
    if (Settings::GlobalScenarios::num_retention +
        Settings::GlobalScenarios::num_creation) {
      if (Settings::GlobalScenarios::num_retention) { // picking SPs
        if (Settings::GlobalScenarios::mood == 3) {
          console->info("   Extracting global SP with optimal mood...");
        } else if (Settings::GlobalScenarios::mood == 1 &&
                   Settings::ImproveFormulations::improve_SP_representation) {
          console->info("   Extracting global SP using MaxCost mood...");
          MaxCost(shared_info, Settings::GlobalScenarios::num_retention,
                  current_directory);
        } else if (Settings::GlobalScenarios::mood == 2 &&
                   Settings::ImproveFormulations::improve_SP_representation) {
          console->info("   Extracting global SP using MinCost mood...");
          MinCost(shared_info, Settings::GlobalScenarios::num_retention,
                  current_directory);
        } else if (Settings::GlobalScenarios::mood == 0 ||
                   !Settings::ImproveFormulations::improve_SP_representation) {
          console->info("   Extracting global SP using random mood...");
          RandomSelection(shared_info,
                          Settings::GlobalScenarios::num_retention);
        } else {
          const bool wrong_setting = false;
          assert(wrong_setting);
        }
      }
      { // creating SPs
        if (Settings::GlobalScenarios::num_creation) {
          GetArtificialSubproblemWeights(shared_info, solver_info_);
        }
      }
      { // adding the chosen ones
        console->info("    Adding these subproblems to master model:");
        if (Settings::GlobalScenarios::num_creation) {
          console->info("     Art_SP");
          AddArtSPTOMP(model_directory, shared_info, solver_info_,
                       master_model_);
        }
        for (const auto sp_id : shared_info.retained_subproblem_ids) {
          console->info("     SP_" + std::to_string(sp_id));
          AddSPToMP(sp_id, shared_info, master_model_, current_directory);
        }
      }
    } // end if
  }

  bool CheckStoppingConditions(const std::shared_ptr<spdlog::logger> console) {
    // std::cout << solver_info_.lp_phase_UB << " " << solver_info_.LB << '\n';
    const double gap =
        100 * std::fabs((solver_info_.lp_phase_UB - solver_info_.LB) /
                        (1e-7 + solver_info_.lp_phase_UB));
    if (gap <= Settings::StoppingConditions::root_node_optimality_gap) {
      console->info("   +Terminating LP phase because it is optimal!");
      return true;
    } else if (solver_info_.duration >=
               Settings::StoppingConditions::root_node_time_limit) {
      console->info("   +Terminating LP phase because time is up!");
      return true;
    } else if (solver_info_.iteration >=
               Settings::StoppingConditions::max_num_iterations) {
      console->info(
          "   +Terminating because maximum number of iterations reached!");
      return true;
    } else if (solver_info_.num_eq_iterations >=
               Settings::StoppingConditions::
                   max_num_non_improvement_iterations) {
      console->info(
          "   +Terminating LP phase because the LB didn't improve for " +
          std::to_string(solver_info_.num_eq_iterations) + " iterations!");
      return true;
    }
    return false;
  }

  void CheckCorrectness(const std::shared_ptr<spdlog::logger> console,
                        SharedInfo &shared_info) {
    /*
      IloNumArray previous_solution = shared_info.master_variables_value;
        // checking if the solution or lb has improved
        if (solver_info_.current_LB == solver_info_.LB) {
          bool warn = true;
          for (uint64_t i = 0; i < previous_solution.getSize();
               ++i) { // check if the solution is the same
            if (std::fabs(previous_solution[i] -
                          shared_info.master_variables_value[i]) > 1e-15) {
              warn = false;
              break;
            }
          }
          if (warn) {
            console->warn(
                "    --The LB didn't improve and the solution didn't change!!");
          }
        }
        previous_solution.end();
    */
  }

  void ExtractVariablesValue(const std::shared_ptr<spdlog::logger> console,
                             SharedInfo &shared_info) {
    if (!Settings::WarmStart::run_ws ||
        solver_info_.iteration >= Settings::WarmStart::num_ws_iterations) {
      master_model_.cplex.getValues(shared_info.master_variables_value,
                                    master_model_.master_variables);
      master_model_.cplex.getValues(shared_info.recourse_variables_value,
                                    master_model_.recourse_variables);
    } else {
      console->info("     Using WS solution to generate cuts...");
      SimpleWS(solver_info_.iteration, master_model_, shared_info);
    }
  }

  void UpdateLBStats(const std::shared_ptr<spdlog::logger> console) {
    solver_info_.current_LB = master_model_.cplex.getObjValue();
    //! Checking if the lb has improved enough
    if (solver_info_.current_LB < solver_info_.LB) {
      console->warn("LB improved by " +
                    std::to_string(solver_info_.current_LB - solver_info_.LB));
    }
    if (solver_info_.current_LB <
        solver_info_.LB + Settings::StoppingConditions::min_lb_improvement) {
      solver_info_.num_eq_iterations++;
    } else {
      solver_info_.num_eq_iterations = 0;
    }
    solver_info_.LB = solver_info_.current_LB;
  }

  bool UpdateUBStats(const SharedInfo &shared_info) {
    if (Settings::WarmStart::run_ws &&
        solver_info_.iteration < Settings::WarmStart::num_ws_iterations) {
      solver_info_.current_UB = IloInfinity;
      return false;
    }
    solver_info_.current_UB = 0;
    // getting second-stage costs
    for (uint64_t sp_id = 0; sp_id < shared_info.subproblem_status.size();
         ++sp_id) {
      if (shared_info.retained_subproblem_ids.count(sp_id)) {
        solver_info_.current_UB += shared_info.subproblem_weight[sp_id] *
                                   shared_info.recourse_variables_value[sp_id];
        continue;
      }
      if (!shared_info.subproblem_status[sp_id]) {
        solver_info_.current_UB += IloInfinity;
        return false;
      }
      solver_info_.current_UB += shared_info.subproblem_weight[sp_id] *
                                 shared_info.subproblem_objective_value[sp_id];
    }
    // get first-stage cost
    solver_info_.current_UB +=
        master_model_.cplex.getObjValue() -
        IloScalProd(shared_info.subproblem_weight,
                    shared_info.recourse_variables_value);
    assert(solver_info_.current_UB < IloInfinity);

    if (solver_info_.current_UB < solver_info_.lp_phase_UB) {
      solver_info_.lp_phase_UB = solver_info_.current_UB;
    }
  }

  void Cleaner(const std::shared_ptr<spdlog::logger> console) {
    int tolerance;
    switch (Settings::Cleaner::aggressiveness) {
    case 0:
      tolerance = 1000;
      break;
    case 1:
      tolerance = 100;
      break;
    case 2:
      tolerance = 10;
      break;
    case 3:
      tolerance = 1;
      break;
    }
    IloRangeArray cuts = IloRangeArray(master_model_.env);
    for (size_t opt_id = 0; opt_id < master_model_.opt_cuts.getSize();
         opt_id++) {
      if (master_model_.cplex.getSlack(master_model_.opt_cuts[opt_id]) >
          tolerance) {
        cuts.add(master_model_.opt_cuts[opt_id]);
      }
    }
    for (size_t fes_id = 0; fes_id < master_model_.feas_cuts.getSize();
         fes_id++) {
      if (master_model_.cplex.getSlack(master_model_.feas_cuts[fes_id]) >
          tolerance) {
        cuts.add(master_model_.feas_cuts[fes_id]);
      }
    }
    console->info("   -Cleaner removed " + std::to_string(cuts.getSize()) +
                  " useless cuts out of " +
                  std::to_string(master_model_.feas_cuts.getSize() +
                                 master_model_.opt_cuts.getSize()));
    cuts.removeFromAll();
    cuts.end();
  }

  void AddCuts(const uint64_t sp_id, const double fixed_part, const bool status,
               const IloNumArray &dual_values) {
    assert(dual_values.getSize() == master_model_.master_variables.getSize());
    IloExpr expr(master_model_.env);
    expr += fixed_part;
    assert(dual_values.getSize() == master_model_.master_variables.getSize());
    expr += IloScalProd(dual_values, master_model_.master_variables);
    if (status) {
      solver_info_.lp_num_opt++;
      expr -= master_model_.recourse_variables[sp_id];
      master_model_.opt_cuts.add(
          IloRange(master_model_.env, -IloInfinity, expr, 0));
    } else {
      solver_info_.lp_num_feas++;
      // assert(fixed_part > 0);
      master_model_.feas_cuts.add(
          IloRange(master_model_.env, -IloInfinity, expr, 0));
    }
    expr.end();
  }

  void SolveRootNode(const std::shared_ptr<spdlog::logger> console,
                     SharedInfo &shared_info,
                     const std::shared_ptr<Subproblem> &SP) {
    console->info("   +Optimizing LP...");
    SetSartTime();
    while (true) {
      if (CheckStoppingConditions(console)) {
        break;
      }
      //! Solve the MP, apply VIs, store solution, etc.
      if (!master_model_.cplex.solve()) {
        if (master_model_.cplex.getStatus() == IloAlgorithm::Infeasible) {
          console->error("   Status of the master problem is infeasible.");
        } else if (master_model_.cplex.getStatus() == IloAlgorithm::Unbounded) {
          console->error("   Status of the master problem is unbounded.");
        } else {
          console->error("   Status of the master problem is unkown.");
        }
        console->warn("------------------------------------------------------");
        console->warn("-Probably suffering from numerics.");
        console->warn(
            "-Maybe turn off AdvInd and/or turn on NumericalEmphasis.");
        console->warn("-I am exporting a MP_.lp for you to examine offline.");
        console->warn("-If the problem presists, contact Ragheb at "
                      "ragheb.rahmaniani@gmail.com.");
        console->warn("______________________________________________________");
        // master_model_.cplex.exportModel("MP_.lp");
        exit(0);
      }
      // master_model_.cplex.exportModel("MP_.lp");
      ExtractVariablesValue(console, shared_info);
      // std::cout << shared_info.master_variables_value << std::endl;
      UpdateLBStats(console);

      //! Generate cuts
      SP->GenBendersCuts(console, shared_info);

      UpdateUBStats(shared_info);

      //! Add cuts to master
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          continue;
        }
        const double fixed_part =
            shared_info.subproblem_objective_value[sp_id] -
            IloScalProd(shared_info.dual_values[sp_id],
                        shared_info.master_variables_value);
        AddCuts(sp_id, fixed_part, shared_info.subproblem_status[sp_id],
                shared_info.dual_values[sp_id]);
      }
      master_model_.model.add(master_model_.opt_cuts);
      master_model_.model.add(master_model_.feas_cuts);

      //! Display current status
      PrintStatus(console);
      solver_info_.iteration++;
      solver_info_.lp_phase_LB = solver_info_.LB;
      if (solver_info_.iteration >=
          Settings::StoppingConditions::max_num_iterations_phase_I) {
        break;
      }
    }
    solver_info_.lp_iteration = solver_info_.iteration;
    master_model_.cplex.solve();
    shared_info.master_vars_reduced_cost = IloNumArray(master_model_.env);
    master_model_.cplex.getReducedCosts(shared_info.master_vars_reduced_cost,
                                        master_model_.master_variables);
    solver_info_.lp_time = GetDuration();
  }

  void ConvertLPtoMIP() {
    master_model_.model.add(IloConversion(
        master_model_.env, master_model_.master_variables, ILOINT));
  }

  /*This func prepares for parallelization of the MP
    It  creates two branches, export one in LP master_formulation
    One thread reads and solves that branch and this processor continues on this
    branch*/
  bool ParallelPreparer(const std::shared_ptr<spdlog::logger> console,
                        SharedInfo &shared_info,
                        const uint64_t num_branches = 2) {
    console->error("I don't support parallelism of the MP yet, sorry!");
    return false;
  }

  bool RunAsHeuristic(const std::shared_ptr<spdlog::logger> console,
                      SharedInfo &shared_info) {
    if (Settings::Heuristic::frequency < 0) {
      return false;
    }
    console->warn("Activating the heuristic, I wont guarantee an optimal "
                  "or even a feasible solution, although do my best");
    double tolerance = Settings::Heuristic::aggressiveness;
    uint64_t num_fixed_lb{0};
    uint64_t num_fixed_ub{0};
    for (int var_id = 0; var_id < shared_info.master_variables_value.getSize();
         ++var_id) {
      if (std::fabs(shared_info.master_variables_value[var_id] -
                    master_model_.master_variables[var_id].getLB()) <
          tolerance) {
        master_model_.master_variables[var_id].setUB(
            master_model_.master_variables[var_id].getLB());
        ++num_fixed_lb;
      } else if (std::fabs(shared_info.master_variables_value[var_id] -
                           master_model_.master_variables[var_id].getUB()) <
                 tolerance) {
        master_model_.master_variables[var_id].setLB(
            master_model_.master_variables[var_id].getUB());
        ++num_fixed_ub;
      }
    }
    console->info("   Heuristic could decide on fate of " +
                  std::to_string(num_fixed_lb + num_fixed_ub) +
                  " master variables out of " +
                  std::to_string(shared_info.master_variables_value.getSize()));
    console->info("    -num_fixed_lb= " + std::to_string(num_fixed_lb) +
                  " num_fixed_ub= " + std::to_string(num_fixed_ub));
  }

  void SolveWithCplexBenders(const std::shared_ptr<spdlog::logger> console) {
    assert(Settings::GlobalScenarios::num_retention ==
           master_model_.recourse_variables.getSize());
    ConvertLPtoMIP();
    master_model_.cplex.setParam(IloCplex::Param::Threads,
                                 Settings::Parallelization::num_proc);
    master_model_.cplex.setParam(
        IloCplex::Param::TimeLimit,
        Settings::StoppingConditions::branching_time_limit);
    master_model_.cplex.setParam(IloCplex::Param::Benders::Strategy, 3);
    if (!master_model_.cplex.solve()) {
      console->error("Failed to solve MIP model with Cplex Benders, please "
                     "examine the exported "
                     "MP_Cplex.lp model");
      master_model_.cplex.exportModel("MP_Cplex.lp");
      exit(0);
    }
    solver_info_.global_UB = master_model_.cplex.getObjValue();
    solver_info_.LB = master_model_.cplex.getBestObjValue();
    solver_info_.num_nodes = master_model_.cplex.getNnodes64();
  }

  void SolveWithCplexBC(const std::shared_ptr<spdlog::logger> console) {
    assert(Settings::GlobalScenarios::num_retention ==
           master_model_.recourse_variables.getSize());
    ConvertLPtoMIP();
    master_model_.cplex.setParam(IloCplex::Param::Threads,
                                 Settings::Parallelization::num_proc);
    master_model_.cplex.setParam(
        IloCplex::Param::TimeLimit,
        Settings::StoppingConditions::branching_time_limit);

    // master_model_.cplex.setParam(IloCplex::Param::Preprocessing::Linear, 1);
    // master_model_.cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 1);

    if (!master_model_.cplex.solve()) {
      console->error(
          "Failed to solve MIP model with B&C, please examine the exported "
          "MP_BC.lp model");
      master_model_.cplex.exportModel("MP_BC.lp");
      exit(0);
    }
    solver_info_.global_UB = master_model_.cplex.getObjValue();
    solver_info_.LB = master_model_.cplex.getBestObjValue();
    solver_info_.num_nodes = master_model_.cplex.getNnodes64();
  }

  // this func use the sols obbtined from the LR cuts to gen cuts and bounds
  bool RunLRBasedHeuristic(SharedInfo &shared_info,
                           const std::shared_ptr<spdlog::logger> console,
                           const std::shared_ptr<Subproblem> &SP) {
    if (!Settings::CutGeneration::use_LR_cuts) {
      console->error(
          "Please turn on the use_LR_cuts to use run_lagrang_heuristic");
      return false;
    }
    master_model_.cplex.setParam(IloCplex::NodeLim,
                                 0); // need to run LR and return
    if (!master_model_.cplex.solve()) {
      console->error(
          "Something is wrong, please examine the exported MP_LR_.lp ");
      master_model_.cplex.exportModel("MP_LR_.lp");
      exit(0);
    }

    Heuristic heur;
    heur.SetVarValFreq(console, shared_info);
    heur.SetNumSolsInPool();
    for (uint64_t itr = 0; itr < Settings::Heuristic::run_lagrang_heuristic;
         ++itr) {
      console->info("  Heuristic searching...");
      // heur.GenSol();
      for (size_t var_id = 0; var_id < master_model_.master_variables.getSize();
           var_id++) {
        shared_info.master_variables_value[var_id] = heur.GenRandVarVal(var_id);
      }
      SP->GenBendersCuts(console, shared_info);
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          continue;
        }
        const double fixed_part =
            shared_info.subproblem_objective_value[sp_id] -
            IloScalProd(shared_info.dual_values[sp_id],
                        shared_info.master_variables_value);
        AddCuts(sp_id, fixed_part, shared_info.subproblem_status[sp_id],
                shared_info.dual_values[sp_id]);
      }
      master_model_.model.add(master_model_.opt_cuts);
      master_model_.model.add(master_model_.feas_cuts);
      // std::cout << shared_info.master_variables_value << std::endl;
      // for (size_t var_id = 0; var_id <
      // master_model_.master_variables.getSize();
      //      var_id++) {
      //   for (size_t val = master_model_.master_variables[var_id].getLB();
      //        val <= master_model_.master_variables[var_id].getUB(); val++) {
      //     // heur.PrintVarValsFreq(console, 0);
      //     std::cout << heur.GetValFreq(var_id, val) << std::endl;
      //   }
      // }
    }

    master_model_.cplex.setParam(IloCplex::NodeLim,
                                 Settings::StoppingConditions::node_limit);
  }

  void SetCallbacks(const std::shared_ptr<spdlog::logger> &console,
                    SharedInfo &shared_info,
                    const std::shared_ptr<Subproblem> &SP) {
    console->info("  -Setting the callbacks...");
    master_model_.cplex.setParam(
        IloCplex::Param::TimeLimit,
        Settings::StoppingConditions::branching_time_limit);
    master_model_.cplex.setParam(IloCplex::Param::Threads, 1);
    // ****Legacy callbacks
    // master_model_.cplex.use(
    //     BendersUserCallback(master_model_.env, console, shared_info, SP,
    //                         master_model_.master_variables,
    //                         master_model_.recourse_variables, solver_info_));
    // master_model_.cplex.use(
    //     BendersLazyCallback(master_model_.env, console, shared_info, SP,
    //                         master_model_.master_variables,
    //                         master_model_.recourse_variables, solver_info_));

    // ****Generic callbacks
    console->info("  -Done.");
  }

  void BranchingPhase(const std::shared_ptr<spdlog::logger> console,
                      SharedInfo &shared_info,
                      const std::shared_ptr<Subproblem> &SP) {
    assert(shared_info.retained_subproblem_ids.size() !=
           shared_info.num_subproblems);

    SetCallbacks(console, shared_info, SP);
    BendersCustomCutCallback generic_cut_callback_(
        console, shared_info, master_model_.master_variables,
        master_model_.recourse_variables, solver_info_, SP);
    contextmask_ = IloCplex::Callback::Context::Id::Candidate;
    // |
    // IloCplex::Callback::Context::Id::Relaxation;
    master_model_.cplex.use(&generic_cut_callback_, contextmask_);

    if (Settings::Heuristic::run_lagrang_heuristic) {
      RunLRBasedHeuristic(shared_info, console, SP);
    }

    console->info("  +Setting up the branch-and-bound tree...");
    if (!master_model_.cplex.solve()) {
      console->error("Failed to solve MIP master, please examine the exported "
                     "MP_BCompose.lp model");
      master_model_.cplex.exportModel("MP_BCompose.lp");
      exit(0);
    }
    solver_info_ = generic_cut_callback_.GetSolverInfo();
    solver_info_.global_UB = master_model_.cplex.getObjValue();
    solver_info_.LB = master_model_.cplex.getBestObjValue();
    solver_info_.num_nodes = master_model_.cplex.getNnodes64();
    PrintStatus(console);

    // master_model_.cplex.getValues(master_model_.master_variables,
    //                               shared_info.master_variables_value);
    // std::cout << shared_info.master_variables_value << std::endl;
  }

  void
  PrintStatus(const std::shared_ptr<spdlog::logger> console) noexcept(true) {
    assert(solver_info_.global_UB >= solver_info_.LB - 1e-7);
    std::string optimization_status = "    ";
    optimization_status +=
        " Iteration=" + std::to_string(solver_info_.iteration);
    optimization_status += " UB=" + ValToStr(solver_info_.global_UB);
    optimization_status += " LB=" + ValToStr(solver_info_.LB) +
                           " LP_UB=" + ValToStr(solver_info_.lp_phase_UB);
    optimization_status +=
        " LP_Gap=" +
        ValToStr((100 * (solver_info_.lp_phase_UB - solver_info_.LB) /
                  std::fabs(solver_info_.lp_phase_UB + 1e-7))) +
        "% Gap=" +
        ValToStr((100 * (solver_info_.global_UB - solver_info_.LB) /
                  std::fabs(solver_info_.global_UB + 1e-7)));
    optimization_status += "% Time=" + ValToStr(GetDuration());
    console->info(optimization_status);
  }

  inline void PrintFinalStats(
      const std::shared_ptr<spdlog::logger> console) noexcept(true) {
    console->info(" ");
    console->info("***********************************");
    console->info("SOLVER:");
    if (Settings::Solver::solver == 0) {
      console->info("  -IloB&C.");
    } else if (Settings::Solver::solver == 1) {
      console->info("  -IloBenders.");
    } else {
      console->info("  -BCompose.");
    }
    console->info("TIME(s):");
    console->info("  -Initialization    = " + ValToStr(solver_info_.init_time));
    console->info("  -1st phase         = " + ValToStr(solver_info_.lp_time));
    console->info("  -2nd phase         = " +
                  ValToStr(GetDuration() - solver_info_.lp_time));
    console->info("  -Total             = " +
                  ValToStr(GetDuration() + solver_info_.init_time));
    console->info("BOUNDS:");
    console->info("  -Root bound        = " +
                  ValToStr(solver_info_.lp_phase_LB));
    console->info("   -Distance from LB = " +
                  ValToStr(100 * (solver_info_.LB - solver_info_.lp_phase_LB) /
                           solver_info_.LB) +
                  "%");
    console->info("  -Root after lifter = " +
                  ValToStr(solver_info_.LB_after_lifter));
    console->info(
        "   -Distance from LB = " +
        ValToStr(100 * (solver_info_.LB - solver_info_.LB_after_lifter) /
                 solver_info_.LB) +
        "%");
    console->info("  -LB                = " + ValToStr(solver_info_.LB));
    console->info("  -UB                = " + ValToStr(solver_info_.global_UB));
    console->info("  -Termination Gap   = " +
                  ValToStr((100 * (solver_info_.global_UB - solver_info_.LB) /
                            (solver_info_.global_UB + 1e-7))) +
                  "%");
    console->info("ITERATIONS:");
    console->info("  -1st phase         = " +
                  std::to_string(solver_info_.lp_iteration));
    console->info(
        "  -2nd phase         = " +
        std::to_string(solver_info_.iteration - solver_info_.lp_iteration));
    console->info("TREE:");
    console->info("  -#Nodes            = " +
                  std::to_string(solver_info_.num_nodes));
    console->info("CUTS:");
    console->info(
        "  -#Feas             = " +
        std::to_string(solver_info_.lp_num_feas + solver_info_.num_feas));
    console->info("    -1st phase       = " +
                  std::to_string(solver_info_.lp_num_feas));
    console->info("    -2nd phase       = " +
                  std::to_string(solver_info_.num_feas));
    console->info(
        "  -#Opt              = " +
        std::to_string(solver_info_.lp_num_opt + solver_info_.num_opt));
    console->info("    -1st phase       = " +
                  std::to_string(solver_info_.lp_num_opt));
    console->info("    -2nd phase       = " +
                  std::to_string(solver_info_.num_opt));
    console->info(
        "  -Sum               = " +
        std::to_string(solver_info_.num_opt + solver_info_.lp_num_opt +
                       solver_info_.num_feas + solver_info_.lp_num_feas));

    console->info("MEMORY:");
    console->info("  -Usage             = " + CurrentRssHuman());
    console->info("***********************************");
    console->info(" ");
  }

  //! Get-Set methods
  inline void SetInitTime() noexcept(true) {
    solver_info_.init_time = GetDuration();
  }
  inline void SetSartTime() noexcept(true) {
    solver_info_.start_time = std::chrono::steady_clock::now();
  }
  inline double GetDuration() noexcept(true) {
    std::chrono::duration<float> diff =
        std::chrono::steady_clock::now() - solver_info_.start_time;
    solver_info_.duration = diff.count();
    return solver_info_.duration;
  }

private:
  MasterModel master_model_;
  MasterSolverInfo solver_info_;
  uint64_t num_threads_ = 1;
  //
  CPXLONG contextmask_ = 0;
  //
  Master(const Master &copy);
};

#endif
