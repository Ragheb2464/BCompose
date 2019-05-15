#ifndef CONTRIB_MP_MASTER_H
#define CONTRIB_MP_MASTER_H
/** @file master.h
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates the parallelization framework.
*/

#include <algorithm>

#include "../../solver_settings.h"
#include "../SP/subproblem.h"
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
  Master() = default;
  ~Master() { master_model_.env.end(); }

  void Initializer(const std::shared_ptr<spdlog::logger> console,
                   SharedInfo &shared_info,
                   const std::string &current_directory) {
    SetSartTime();
    //! Load the model
    const bool loading_status =
        Loader(console, master_model_, shared_info, current_directory);
    if (!loading_status) {
      console->info("Failed to load the MP.sav!");
      exit(0);
    }

    //! initialize the params
    shared_info.master_variables_value =
        IloNumArray(master_model_.env, shared_info.num_master_variables);
    shared_info.recourse_variables_value =
        IloNumArray(master_model_.env, shared_info.num_recourse_variables);
    shared_info.num_subproblems = shared_info.num_recourse_variables;
  }

  void PartialDecompsoition(const std::string &model_directory,
                            const std::shared_ptr<spdlog::logger> console,
                            SharedInfo &shared_info,
                            const std::string &current_directory) {
    assert(shared_info.num_subproblems >= _num_retention);
    if (_num_creation && (shared_info.num_subproblems == 1 ||
                          _num_retention == shared_info.num_subproblems ||
                          _solver == 0 || _solver == 1)) {
      console->error("Please turn off creation strategy.");
      exit(911);
    }
    if (_solver == 0 || _solver == 1) {
      assert(!_num_creation);
      SelectAll(shared_info);
    } else {
      if (_num_retention) { // picking SPs
        if (_mood == 0 || !_improve_SP_representation) {
          console->info("   Extracting global SP using random strategy...");
          RandomSelection(shared_info, _num_retention);
        } else if (_mood == 1 && _improve_SP_representation) {
          console->info("   Extracting global SP using MaxCost strategy...");
          MaxCost(shared_info, _num_retention, current_directory);
        } else if (_mood == 2 && _improve_SP_representation) {
          console->info("   Extracting global SP using MinCost strategy...");
          MinCost(shared_info, _num_retention, current_directory);
        } else if (_mood == 3) {
          console->info("   Extracting global SP with optimal strategy...");
        } else {
          console->error("Please choose right strategy for the PD.");
          exit(911);
        }
      }
      if (_num_creation) { // creating SPs
        GetArtificialSubproblemWeights(shared_info, solver_info_);
      }
    }
    { // adding the chosen ones
      console->info("    Rebuilding the master model...");
      if (_num_creation) {
        console->info("    +Art_SP");
        AddArtSPToMP(model_directory, shared_info, solver_info_, master_model_);
      }
      for (const uint64_t sp_id : shared_info.retained_subproblem_ids) {
        console->info("    +SP_" + std::to_string(sp_id));
        AddSPToMP(sp_id, master_model_, current_directory);
      }
    } // end if

    console->info("   ->PD went through successfully.");
  }

  inline bool
  CheckStoppingConditions(const std::shared_ptr<spdlog::logger> console) {
    // std::cout << solver_info_.lp_phase_UB << " " << solver_info_.LB << '\n';
    const double gap =
        std::min(100 * std::fabs((solver_info_.lp_phase_UB - solver_info_.LB) /
                                 (1e-7 + solver_info_.lp_phase_UB)),
                 100.0);
    if (solver_info_.iteration > 1 && gap <= _root_node_optimality_gap) {
      console->info("   +Terminating LP phase because gap is " + ValToStr(gap));
      return true;
    } else if (solver_info_.duration >= _root_node_time_limit) {
      console->info("   +Terminating LP phase because time is up!");
      return true;
    } else if (solver_info_.iteration >= _max_num_iterations) {
      console->info(
          "   +Terminating because maximum number of iterations reached!");
      return true;
    } else if (solver_info_.num_eq_iterations >=
               _max_num_non_improvement_iterations) {
      console->info(
          "   +Terminating LP phase because the LB didn't improve for " +
          std::to_string(solver_info_.num_eq_iterations) + " iterations!");
      return true;
    }
    return false;
  }

  static void CheckCorrectness(const std::shared_ptr<spdlog::logger> console,
                               SharedInfo &shared_info) {
    /*  IloNumArray previous_solution = shared_info.master_variables_value;
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
      previous_solution.end();*/
  }

  inline void
  ExtractVariablesValue(const std::shared_ptr<spdlog::logger> console,
                        SharedInfo &shared_info) {
    if (!_run_ws || solver_info_.iteration >= _num_ws_iterations) {
      master_model_.cplex.getValues(shared_info.master_variables_value,
                                    master_model_.master_variables);
      master_model_.cplex.getValues(shared_info.recourse_variables_value,
                                    master_model_.recourse_variables);
    } else {
      console->info("    +Using WS solution to generate cuts...");
      SimpleWS(solver_info_.iteration, master_model_, shared_info);
    }
  }

  inline void UpdateLBStats(const std::shared_ptr<spdlog::logger> console) {
    solver_info_.current_LB = master_model_.cplex.getObjValue();
    //! Checking if the lb has improved enough
    if (solver_info_.current_LB < solver_info_.LB) {
      console->warn("LB improved by " +
                    std::to_string(solver_info_.current_LB - solver_info_.LB));
    }
    if (solver_info_.current_LB < solver_info_.LB + _min_lb_improvement) {
      solver_info_.num_eq_iterations++;
    } else {
      solver_info_.num_eq_iterations = 0;
    }
    solver_info_.LB = solver_info_.current_LB;
  }

  inline bool UpdateUBStats(const SharedInfo &shared_info) {
    if (_run_ws && solver_info_.iteration < _num_ws_iterations) {
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
    return true;
  }

  void Cleaner(const std::shared_ptr<spdlog::logger> console) {
    int tolerance;
    switch (_cleaner_aggressiveness) {
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
    IloNumArray slacks_val(master_model_.env, master_model_.opt_cuts.getSize());
    master_model_.cplex.getSlacks(slacks_val, master_model_.opt_cuts);
    for (IloInt opt_id = 0; opt_id < master_model_.opt_cuts.getSize();
         ++opt_id) {
      if (slacks_val[opt_id] > tolerance) {
        cuts.add(master_model_.opt_cuts[opt_id]);
      }
    }
    slacks_val.end();
    IloNumArray slacks_val_feas(master_model_.env,
                                master_model_.feas_cuts.getSize());
    master_model_.cplex.getSlacks(slacks_val_feas, master_model_.feas_cuts);
    for (IloInt fes_id = 0; fes_id < master_model_.feas_cuts.getSize();
         ++fes_id) {
      if (slacks_val_feas[fes_id] > tolerance) {
        cuts.add(master_model_.feas_cuts[fes_id]);
      }
    }
    slacks_val_feas.end();
    console->info("   -Cleaner removed " + std::to_string(cuts.getSize()) +
                  " useless cuts out of " +
                  std::to_string(master_model_.feas_cuts.getSize() +
                                 master_model_.opt_cuts.getSize()));
    cuts.removeFromAll();
    cuts.endElements();
    cuts.end();
  }

  inline void AddCuts(const uint64_t sp_id, const double fixed_part,
                      const bool status, const IloNumArray &dual_values) {
    assert(dual_values.getSize() == master_model_.master_variables.getSize());
    IloExpr expr(master_model_.env);
    expr += fixed_part;
    assert(dual_values.getSize() == master_model_.master_variables.getSize());
    expr += IloScalProd(dual_values, master_model_.master_variables);
    if (status) {
      ++solver_info_.lp_num_opt;
      expr -= master_model_.recourse_variables[sp_id];
      master_model_.opt_cuts.add(
          IloRange(master_model_.env, -IloInfinity, expr, 0));
    } else {
      ++solver_info_.lp_num_feas;
      // assert(fixed_part > 0);
      master_model_.feas_cuts.add(
          IloRange(master_model_.env, -IloInfinity, expr, 0));
    }
    expr.end();
  }

  void SolveRootNode(const std::shared_ptr<spdlog::logger> console,
                     SharedInfo &shared_info,
                     const std::shared_ptr<Subproblem> &SP) {
    console->info("  *Running master on " + std::to_string(num_threads_) +
                  " core.");
    master_model_.cplex.setParam(IloCplex::Param::Threads, num_threads_);
    SP->SetNumThreads(
        console,
        std::min(shared_info.num_subproblems -
                     shared_info.retained_subproblem_ids.size() + 0.0,
                 _num_worker_processors + _num_master_processors - 1.0));

    console->info("Optimizing the LP...");
    SetSartTime();
    while (true) {
      if (CheckStoppingConditions(console)) {
        break;
      }
      //! Solve the MP, apply VIs, store solution, etc.
      if (!master_model_.cplex.solve()) {
        if (master_model_.cplex.getStatus() == IloAlgorithm::Infeasible) {
          console->error("   Master problem is infeasible.");
        } else if (master_model_.cplex.getStatus() == IloAlgorithm::Unbounded) {
          console->error("   Master problem is unbounded.");
        } else {
          console->error("   Master problem is unkown.");
        }
        console->warn("------------------------------------------------------");
        console->warn("-Probably suffering from numerics.");
        console->warn(
            "-Maybe turn off AdvInd and/or turn on NumericalEmphasis.");
        console->warn("-I am exporting a MP_.lp for you to examine offline.");
        console->warn("______________________________________________________");
        master_model_.cplex.exportModel("MP_.lp");
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
        const IloNum fixed_part =
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
      if (solver_info_.iteration >= _max_num_iterations_phase_I) {
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

  inline void ConvertLPtoMIP() {
    master_model_.model.add(IloConversion(
        master_model_.env, master_model_.master_variables, ILOINT));
  }

  bool RunAsHeuristic(const std::shared_ptr<spdlog::logger> console,
                      SharedInfo &shared_info) {
    if (_frequency < 0) {
      return false;
    }
    console->warn("Activating the heuristic, I wont guarantee an optimal "
                  "or even a feasible solution, although do my best");
    double tolerance = _heur_aggressiveness;
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
    return true;
  }

  void SolveWithCplexBenders(const std::shared_ptr<spdlog::logger> console) {
    // assert(_num_retention == master_model_.recourse_variables.getSize());
    ConvertLPtoMIP();
    master_model_.cplex.setParam(IloCplex::Param::Threads,
                                 _num_worker_processors +
                                     _num_master_processors);
    master_model_.cplex.setParam(IloCplex::Param::TimeLimit,
                                 _branching_time_limit);
    master_model_.cplex.setParam(IloCplex::Param::Benders::Strategy, 3);
    // master_model_.cplex.setParam(IloCplex::Param::Preprocessing::Linear, 1);
    // master_model_.cplex.setParam(IloCplex::Param::Preprocessing::Reduce, 1);
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
    // assert(_num_retention == master_model_.recourse_variables.getSize());
    ConvertLPtoMIP();
    master_model_.cplex.setParam(IloCplex::Param::Threads,
                                 _num_worker_processors +
                                     _num_master_processors);
    master_model_.cplex.setParam(IloCplex::Param::TimeLimit,
                                 _branching_time_limit);

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
  void RunLRBasedHeuristic(SharedInfo &shared_info,
                           const std::shared_ptr<spdlog::logger> console,
                           const std::shared_ptr<Subproblem> SP) {
    if (!_use_root_lifter) {
      console->error(
          "Please turn on the use_root_lifter to use run_lagrang_heuristic");
      exit(911);
    }
    master_model_.cplex.setParam(IloCplex::NodeLim,
                                 0); // need to run LR and return
    if (!master_model_.cplex.solve()) {
      console->error(
          "Something is wrong, please examine the exported MP_LR_.lp ");
      master_model_.cplex.exportModel("MP_LR_.lp");
      exit(911);
    }

    Heuristic heur;
    heur.SetVarValFreq(shared_info);
    heur.SetNumSolsInPool();
    for (uint64_t itr = 0; itr < _run_lagrang_heuristic; ++itr) {
      console->info("  Heuristic searching...");
      // heur.GenSol();
      for (IloInt var_id = 0; var_id < master_model_.master_variables.getSize();
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

    master_model_.cplex.setParam(IloCplex::NodeLim, _node_limit);
  }

  inline void SetCallbacks(const std::shared_ptr<spdlog::logger> console,
                           SharedInfo &shared_info,
                           const std::shared_ptr<Subproblem> SP) {
    assert(shared_info.num_subproblems >
           shared_info.retained_subproblem_ids.size());
    const uint64_t num_sp_workers =
        fmin(shared_info.num_subproblems -
                 shared_info.retained_subproblem_ids.size(),
             _num_worker_processors);
    num_threads_ =
        fmax(_num_master_processors,
             _num_master_processors + _num_worker_processors - num_sp_workers);

    console->info("  +Setting up the branch-and-bound tree on " +
                  std::to_string(num_threads_) + " threads.");
    master_model_.cplex.setParam(IloCplex::Param::TimeLimit,
                                 _branching_time_limit);
    master_model_.cplex.setParam(IloCplex::Param::Threads, num_threads_);
    contextmask_ = IloCplex::Callback::Context::Id::Candidate;
    if (_use_root_lifter) {
      contextmask_ |= IloCplex::Callback::Context::Id::Relaxation;
    }

    SP->SetNumThreads(console, num_sp_workers);
  }

  void BranchingPhase(const std::shared_ptr<spdlog::logger> console,
                      SharedInfo &shared_info,
                      const std::shared_ptr<Subproblem> SP) {
    assert(shared_info.retained_subproblem_ids.size() !=
           shared_info.num_subproblems);

    SetCallbacks(console, shared_info, SP);
    BendersCustomCutCallback generic_cut_callback_(
        console, shared_info, master_model_.master_variables,
        master_model_.recourse_variables, solver_info_, SP);
    master_model_.cplex.use(&generic_cut_callback_, contextmask_);

    if (_run_lagrang_heuristic) {
      RunLRBasedHeuristic(shared_info, console, SP);
    }
    console->info(" -Done.");

    console->info("Exploring the tree...");
    // master_model.cplex.setOut(master_model.env.getNullStream());
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

  inline void
  PrintStatus(const std::shared_ptr<spdlog::logger> console) noexcept(true) {
    assert(solver_info_.global_UB >= solver_info_.LB - 1e-7);
    std::string optimization_status = " UB=" + ValToStr(solver_info_.global_UB);
    optimization_status += " LB=" + ValToStr(solver_info_.LB) +
                           " LP_UB=" + ValToStr(solver_info_.lp_phase_UB);
    optimization_status +=
        " LP_Gap=" +
        ValToStr((100 * (solver_info_.lp_phase_UB - solver_info_.LB) /
                  std::fabs(solver_info_.lp_phase_UB + 1e-7))) +
        "% Gap=" +
        ValToStr((100 * (solver_info_.global_UB - solver_info_.LB) /
                  std::fabs(solver_info_.global_UB + 1e-7)));
    optimization_status += "% #Iter=" + std::to_string(solver_info_.iteration);
    optimization_status += " Time=" + ValToStr(GetDuration());
    console->info(optimization_status);
  }

  inline void PrintFinalStats(
      const std::shared_ptr<spdlog::logger> console) noexcept(true) {
    console->info(" ");
    console->info("***********************************");
    console->info("SOLVER:");
    if (_solver == 0) {
      console->info("  -IloB&C.");
    } else if (_solver == 1) {
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
    console->info("   ->Distance from LB= " +
                  ValToStr(100 * (solver_info_.LB - solver_info_.lp_phase_LB) /
                           solver_info_.LB) +
                  "%");
    console->info("  -Root after lifter = " +
                  ValToStr(solver_info_.LB_after_lifter));
    console->info(
        "   ->Distance from LB= " +
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
    console->info("    ->1st phase      = " +
                  std::to_string(solver_info_.lp_num_feas));
    console->info("    ->2nd phase      = " +
                  std::to_string(solver_info_.num_feas));
    console->info(
        "  -#Opt              = " +
        std::to_string(solver_info_.lp_num_opt + solver_info_.num_opt));
    console->info("    ->1st phase      = " +
                  std::to_string(solver_info_.lp_num_opt));
    console->info("    ->2nd phase      = " +
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
