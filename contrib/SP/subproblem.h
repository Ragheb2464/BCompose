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

#include "../../solver_settings.h"
#include "../heuristic/heuristic.h"
#include "../shared_info/structures.h"
#include "utils/LR_cuts.h"
#include "utils/classical_cuts.h"

class Subproblem {
public:
    Subproblem() = default;

    ~Subproblem() {
        for (uint64_t sp_id = 0; sp_id < subproblem_model_.size(); ++sp_id) {
            subproblem_model_[sp_id].env.end();
        }
        subproblem_model_.clear();
    }

    void Initializer(const std::shared_ptr<spdlog::logger> console,
                     SharedInfo &shared_info,
                     const std::string &current_directory) {
        num_subproblems_ = GetSubproblemCount();
        if (num_subproblems_ != shared_info.num_recourse_variables) {
            console->error("   Number of recourse variables (theta_) does not match "
                           "number of subproblems!!");
            exit(0);
        }

        console->info("  ->Loading " + std::to_string(num_subproblems_) +
                      " subpropblems...");
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
        if (_use_root_lifter) { // FOR LR CUTS
            Loader(console, LR_subproblem_model_, shared_info, current_directory);
            shared_info.copied_variables_value.resize(num_subproblems_);
            for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
                LR_subproblem_model_[sp_id].cplex.setParam(IloCplex::Param::TimeLimit,
                                                           _subpproblem_time_limit);

                shared_info.copied_variables_value[sp_id] =
                        IloNumArray(LR_subproblem_model_[sp_id].env);
                if (!LR_subproblem_model_[sp_id].cplex.isMIP()) {
                    LR_subproblem_model_[sp_id].model.add(IloConversion(
                            LR_subproblem_model_[sp_id].env,
                            LR_subproblem_model_[sp_id].copied_variables, ILOINT));
                }
                LR_subproblem_model_[sp_id].NAC_constraints.removeFromAll();
                LR_subproblem_model_[sp_id].NAC_constraints.endElements();
                LR_subproblem_model_[sp_id].NAC_constraints.end();
            }
        }

        shared_info.subproblem_objective_value.resize(num_subproblems_);
        shared_info.subproblem_status.resize(num_subproblems_);
        shared_info.dual_values.resize(num_subproblems_);
        shared_info.copied_variables_value.resize(num_subproblems_);
        for (uint64_t sp_id = 0; sp_id < num_subproblems_; ++sp_id) {
            shared_info.subproblem_objective_value[sp_id] = 0;
            shared_info.subproblem_status[sp_id] = true;
            //
            shared_info.dual_values[sp_id] =
                    IloNumArray(subproblem_model_[sp_id].env,
                                subproblem_model_[sp_id].NAC_constraints.getSize());
            shared_info.copied_variables_value[sp_id] =
                    IloNumArray(subproblem_model_[sp_id].env,
                                subproblem_model_[sp_id].NAC_constraints.getSize());
        }
    }

    inline void SetNumThreads(const std::shared_ptr<spdlog::logger> console,
                              const uint64_t _num_threads) {
        num_threads_ = _num_threads;
        console->info("  *Using up to " + std::to_string(num_threads_) +
                      " cores to generate cuts.");
    }

    inline static void PerturbMasterSolution(SharedInfo &shared_info) {
        const double alpha = _perturbation_weight;
        const double core_val = _initial_core_point;
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
        if (_improve_SP_representation && !_use_root_lifter) {
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
                cuts.endElements();
                cuts.end();
            }
            console->info("   -Cleaner removed " + std::to_string(t) +
                          " useless constraints out of " + std::to_string(num_con));
        } else {
            console->info("   -Nothing to clean");
        }
    }

    inline void GenBendersCuts(const std::shared_ptr<spdlog::logger> console,
                               SharedInfo &shared_info) {
        for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
            shared_info.subproblem_status[sp_id] = false;
            shared_info.dual_values[sp_id].clear();
        }

        {
            std::ios_base::sync_with_stdio(false);
            std::cin.tie(nullptr);
            // Now, we create the threadpool.
            boost::asio::io_service io_service(num_threads_);
            std::unique_ptr<boost::asio::io_service::work> work_(
                    new boost::asio::io_service::work(io_service));
            boost::thread_group thread_pool;
            for (unsigned t = 0; t < num_threads_; ++t) {
                thread_pool.create_thread(
                        boost::bind(&boost::asio::io_service::run, &io_service));
            }
            // dispatch SPs
            for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
                if (shared_info.retained_subproblem_ids.count(sp_id)) {
                    continue;
                }
                subproblem_model_[sp_id].NAC_constraints.setBounds(
                        shared_info.master_variables_value,
                        shared_info.master_variables_value);
                io_service.dispatch(boost::bind(
                        GenClassicalCuts, &subproblem_model_[sp_id], &shared_info, sp_id));
            }
            // We let the thread pool finish the computation.
            work_.reset();
            thread_pool.join_all();
            io_service.stop();
        }

        // get some stats
        uint64_t num_opt_cut = 0;
        uint64_t num_feas_cut = 0;
        for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
            if (shared_info.retained_subproblem_ids.count(sp_id)) {
                continue;
            }
            assert(shared_info.dual_values[sp_id].getSize());
            if (shared_info.subproblem_objective_value[sp_id] <= 0 &&
                !shared_info.subproblem_status[sp_id]) {
                console->error(
                        "The subproblem is infeasible but the feasibility measure is " +
                        std::to_string(shared_info.subproblem_objective_value[sp_id]) +
                        ". Please examine the exported SP_numeric.lp");
                subproblem_model_[sp_id].cplex.exportModel("SP_numeric.lp");
                exit(911);
            }
            num_opt_cut += shared_info.subproblem_status[sp_id];
            num_feas_cut += 1 - shared_info.subproblem_status[sp_id];
        }
        assert(num_opt_cut + num_feas_cut +
               shared_info.retained_subproblem_ids.size() ==
               shared_info.num_subproblems);
        console->info("   ->Generated " + std::to_string(num_opt_cut) +
                      " optimality cuts and " + std::to_string(num_feas_cut) +
                      " feasbility cuts.");
        ++solver_info_.iteration;
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
            std::unique_ptr<boost::asio::io_service::work> work_(
                    new boost::asio::io_service::work(io_service));
            boost::thread_group thread_pool;
            for (size_t i = 0; i < num_threads_; ++i) {
                thread_pool.create_thread(
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
                io_service.dispatch(boost::bind(GenLRCuts, &LR_subproblem_model_[sp_id],
                                                &shared_info, &copy_sol_tmp_vec[sp_id],
                                                sp_id));
            }
            // We let the thread pool finish the computation.
            work_.reset();
            thread_pool.join_all();
            io_service.stop();
            // record the sols
            if (_run_lagrang_heuristic || _run_lagrang_fixer) {
                UpdateSolPool(copy_sol_tmp_vec, shared_info, console);
            }
        }
        solver_info_.iteration++;
    }

    inline void DeleteLRSubproblems() {
        for (uint64_t sp_id = 0; sp_id < LR_subproblem_model_.size(); ++sp_id) {
            LR_subproblem_model_[sp_id].env.end();
        }
        LR_subproblem_model_.clear();
    }

    std::vector<SubproblemModel> subproblem_model_;
    std::vector<SubproblemModel> LR_subproblem_model_;
    SubproblemSolverInfo solver_info_;

private:
    uint64_t num_subproblems_ = 0, num_threads_ = 1;
    std::mutex mtx_;

    //
    Subproblem(const Subproblem &);
};

#endif
