#ifndef CONTRIB_SP_SUBPROBLEM_H
#define CONTRIB_SP_SUBPROBLEM_H

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>

#include "structures.h"

#include "loader.h"

#include "../control/solver_settings.h"
#include "../heuristic/heuristic.h"
#include "../shared_info/structures.h"
#include "utils/LR_cuts.h"
#include "utils/classical_cuts.h"

class Subproblem {
public:
  Subproblem() {}
  ~Subproblem() {}

  void Initializer(const std::shared_ptr<spdlog::logger> console,
                   SharedInfo &shared_info,
                   const std::string &current_directory) {
    num_subproblems_ = GetSubproblemCount();
    if (num_subproblems_ != shared_info.num_recourse_variables) {
      console->error("   Number of recourse variables (theta_) does not match "
                     "number of subproblems!!");
      exit(0);
    }

    console->info("   Loading " + std::to_string(num_subproblems_) +
                  " subpropblems...");
    //! TODO: Make it possible to load only one subproblem
    { // loading LP SPs
      Loader(console, subproblem_model_, shared_info, current_directory);
      for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
        if (subproblem_model_[sp_id].cplex.isMIP()) {
          subproblem_model_[sp_id].model.add(IloConversion(
              subproblem_model_[sp_id].env,
              subproblem_model_[sp_id].copied_variables, ILOFLOAT));
        }
      }
    }
    if (Settings::CutGeneration::use_LR_cuts) { // FOR LR CUTS
      Loader(console, LR_subproblem_model_, shared_info, current_directory);
      shared_info.copied_variables_value.resize(num_subproblems_);
      for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
        LR_subproblem_model_[sp_id].cplex.setParam(
            IloCplex::Param::TimeLimit,
            Settings::StoppingConditions::subpproblem_time_limit);

        shared_info.copied_variables_value[sp_id] =
            IloNumArray(LR_subproblem_model_[sp_id].env);
        if (!LR_subproblem_model_[sp_id].cplex.isMIP()) {
          LR_subproblem_model_[sp_id].model.add(IloConversion(
              LR_subproblem_model_[sp_id].env,
              LR_subproblem_model_[sp_id].copied_variables, ILOINT));
        }
        LR_subproblem_model_[sp_id].NAC_constraints.removeFromAll();
        LR_subproblem_model_[sp_id].NAC_constraints.end();
      }
    }

    shared_info.subproblem_objective_value.resize(num_subproblems_);
    shared_info.subproblem_status.resize(num_subproblems_);
    shared_info.dual_values.resize(num_subproblems_);
    shared_info.copied_variables_value.resize(num_subproblems_);
    for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
      shared_info.dual_values[sp_id] =
          IloNumArray(subproblem_model_[sp_id].env,
                      subproblem_model_[sp_id].NAC_constraints.getSize());
      shared_info.copied_variables_value[sp_id] =
          IloNumArray(subproblem_model_[sp_id].env,
                      subproblem_model_[sp_id].NAC_constraints.getSize());
    }

    num_threads_ = static_cast<uint64_t>(
        std::min(std::thread::hardware_concurrency() + 0.0,
                 shared_info.num_subproblems -
                     shared_info.retained_subproblem_ids.size() + 0.0),
        Settings::Parallelization::num_proc);
    console->info("    Using up to " + std::to_string(num_threads_) +
                  " cores to generate cuts.");
  }

  static void PerturbMasterSolution(SharedInfo &shared_info) {
    const double alpha = Settings::CutGeneration::perturbation_weight;
    const double core_val = Settings::CutGeneration::initial_core_point;
    for (IloInt var_id = 0;
         var_id < shared_info.master_variables_value.getSize(); ++var_id) {
      shared_info.master_variables_value[var_id] = std::min(
          1.0, shared_info.master_variables_value[var_id] + alpha * core_val);
    }
  }

  /*This func removes all the cuts added at the pre-process step. I wont remove
   * the cuts from SP if supposed to gen LR  cuts*/
  void Cleaner(const std::shared_ptr<spdlog::logger> console,
               const SharedInfo &shared_info) {
    if (Settings::ImproveFormulations::improve_SP_representation &&
        !Settings::CutGeneration::use_LR_cuts) {
      int t = 0, num_con = 0;
      for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          continue;
        }
        IloRangeArray cuts = IloRangeArray(subproblem_model_[sp_id].env);
        for (IloInt con_id = 0;
             con_id < subproblem_model_[sp_id].constraints.getSize();
             con_id++) {
          const std::string con_name =
              subproblem_model_[sp_id].constraints[con_id].getName();
          if (con_name.find("Reg_") == std::string::npos) {
            cuts.add(subproblem_model_[sp_id].constraints[con_id]);
            ++t;
          }
          ++num_con;
        }
        cuts.removeFromAll();
        cuts.end();
      }
      console->info("   -Cleaner removed " + std::to_string(t) +
                    " useless constraints out of " + std::to_string(num_con));
    } else {
      console->info("   -Nothing to clean");
    }
  }

  void GenBendersCuts(const std::shared_ptr<spdlog::logger> console,
                      SharedInfo &shared_info) {
    {
      std::ios_base::sync_with_stdio(false);
      std::cin.tie(nullptr);
      // Now, we create the threadpool.
      boost::asio::io_service io_service(num_threads_);
      std::unique_ptr<boost::asio::io_service::work> work(
          new boost::asio::io_service::work(io_service));
      boost::thread_group threads;
      for (size_t i = 0; i < num_threads_; ++i) {
        threads.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service));
      }
      // dispatch SPs
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          shared_info.subproblem_status[sp_id] = true;
          continue;
        }
        subproblem_model_[sp_id].NAC_constraints.setBounds(
            shared_info.master_variables_value,
            shared_info.master_variables_value);
        io_service.dispatch(boost::bind(
            GenClassicalCuts, &subproblem_model_[sp_id], &shared_info, sp_id));
      }
      // We let the thread pool finish the computation.
      work.reset();
      threads.join_all();
      io_service.stop();
    }
    // get some stats
    uint64_t num_opt_cut = 0;
    uint64_t num_feas_cut = 0;
    for (uint32_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
      if (shared_info.retained_subproblem_ids.count(sp_id)) {
        continue;
      }
      if (shared_info.subproblem_objective_value[sp_id] <= -1e-7 &&
          !shared_info.subproblem_status[sp_id]) {
        console->error(
            "I am suffering from numerical issues, the pproblem is infeas but "
            "the feasibility measure is " +
            std::to_string(shared_info.subproblem_objective_value[sp_id]) +
            ". Please examine the exported SP_numeric.lp offline");
        subproblem_model_[sp_id].cplex.exportModel("SP_numeric.lp");
        exit(0);
      }
      num_opt_cut += shared_info.subproblem_status[sp_id];
      num_feas_cut += 1 - shared_info.subproblem_status[sp_id];
    }
    assert(num_opt_cut + num_feas_cut +
               shared_info.retained_subproblem_ids.size() ==
           shared_info.num_subproblems);
    console->info("     Generated " + std::to_string(num_opt_cut) +
                  " optimality cuts and " + std::to_string(num_feas_cut) +
                  " feasbility cuts");
    solver_info_.iteration++;
    // mtx_.unlock();
  }

  static void
  UpdateSolPool(std::vector<std::vector<std::vector<int>>> &copy_sol_tmp_vec,
                SharedInfo &shared_info,
                const std::shared_ptr<spdlog::logger> console) {
    for (size_t sp_id = 0; sp_id < copy_sol_tmp_vec.size(); sp_id++) {
      if (shared_info.retained_subproblem_ids.count(sp_id)) {
        assert(!copy_sol_tmp_vec[sp_id].size());
        continue;
      }
      for (size_t sol_id = 0; sol_id < copy_sol_tmp_vec[sp_id].size();
           sol_id++) {
        CopiedVarsValPool copy_vars_val{};
        copy_vars_val.solution.resize(copy_sol_tmp_vec[sp_id][sol_id].size());
        for (size_t var_id = 0; var_id < copy_sol_tmp_vec[sp_id][sol_id].size();
             ++var_id) {
          copy_vars_val.solution[var_id] =
              copy_sol_tmp_vec[sp_id][sol_id][var_id];
        }
        const auto &res =
            shared_info.copied_vars_val_pool.insert(std::move(copy_vars_val));
        if (!res.second) {
          console->info("seen already");
        }
      }
    }
    console->info("Int pool size= " +
                  std::to_string(shared_info.copied_vars_val_pool.size()));
    // GetVarValFreq(console, shared_info);
  }
  void GenAdvancedCuts(const std::shared_ptr<spdlog::logger> console,
                       SharedInfo &shared_info) {
    {
      std::vector<std::vector<std::vector<int>>> copy_sol_tmp_vec{
          shared_info.num_subproblems};
      std::ios_base::sync_with_stdio(false);
      std::cin.tie(nullptr);
      // Now, we create the threadpool.
      boost::asio::io_service io_service(num_threads_);
      std::unique_ptr<boost::asio::io_service::work> work(
          new boost::asio::io_service::work(io_service));
      boost::thread_group threads;
      for (size_t i = 0; i < num_threads_; ++i) {
        threads.create_thread(
            boost::bind(&boost::asio::io_service::run, &io_service));
      }

      // dispatch SPs
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        if (shared_info.retained_subproblem_ids.count(sp_id)) {
          shared_info.subproblem_status[sp_id] = true;
          continue;
        }
        if (!shared_info.subproblem_status[sp_id]) { // dont lift for infeas SPs
          continue;
        }
        io_service.dispatch(boost::bind(
            GenLRCuts, &subproblem_model_[sp_id], &LR_subproblem_model_[sp_id],
            &shared_info, &copy_sol_tmp_vec[sp_id], sp_id));
      }
      // We let the thread pool finish the computation.
      work.reset();
      threads.join_all();
      io_service.stop();
      // record the sols
      if (Settings::Heuristic::run_lagrang_heuristic ||
          Settings::Heuristic::run_lagrang_fixer) {
        UpdateSolPool(copy_sol_tmp_vec, shared_info, console);
      }
    }
    solver_info_.iteration++;
  }

  void DeleteLRSubproblems() {
    for (uint64_t sp_id = 0; sp_id < LR_subproblem_model_.size(); ++sp_id) {
      LR_subproblem_model_[sp_id].env.end();
    }
    LR_subproblem_model_.clear();
  }

  std::vector<SubproblemModel> subproblem_model_;
  std::vector<SubproblemModel> LR_subproblem_model_;
  SubproblemSolverInfo solver_info_;

private:
  uint64_t num_subproblems_ = 0, num_threads_ = 0;
  std::mutex mtx_;
  //
  Subproblem(const Subproblem &copy);
};
#endif
