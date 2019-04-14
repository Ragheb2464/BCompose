#ifndef COTRIB_PRE_PROCESSOR_MODEL_LIFTINIG_SPs_H
#define COTRIB_PRE_PROCESSOR_MODEL_LIFTINIG_SPs_H

#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include "../control/solver_settings.h"
#include "../shared_info/structures.h"
// Get declaration for lifter(char *export_name)
#include "../pre_processor/model_refiner.h"

void LiftSPs(const std::string current_directory,
             const SharedInfo &shared_info) {
  double *obj_vals =
      (double *)malloc(shared_info.num_subproblems * sizeof(double));
  const size_t num_threads = Settings::Parallelization::num_proc;
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);
  // Now, we create the threadpool.
  boost::asio::io_service io_service(num_threads);
  std::unique_ptr<boost::asio::io_service::work> work(
      new boost::asio::io_service::work(io_service));
  boost::thread_group threads;
  for (size_t i = 0; i < num_threads; ++i) {
    threads.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service));
  }

  std::vector<std::string> dir_vec;
  for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
    dir_vec.push_back(current_directory + "/opt_model_dir/SP_" +
                      std::to_string(sp_id) + ".sav");
  }

  // alloc_vec_size(shared_info.num_subproblems, obj_vals);

  for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
    io_service.dispatch(boost::bind(
        lifter, dir_vec[sp_id].c_str(), sp_id,
        Settings::ImproveFormulations::aggressiveness,
        Settings::StoppingConditions::lifter_time_limit_per_SP, obj_vals));
    // lifter(dir_vec[sp_id].c_str(), sp_id,
    //        Settings::ImproveFormulations::aggressiveness,
    //        Settings::StoppingConditions::lifter_time_limit_per_SP);
  }

  work.reset();
  threads.join_all();
  io_service.stop();

  {
    if (Settings::GlobalScenarios::num_retention) {
      const std::string path_to_SP_data_file =
          current_directory + "/contrib/pre_processor/SP_info.txt";
      std::ofstream file(path_to_SP_data_file, std::ofstream::trunc);
      // file.open(path_to_SP_data_file);
      assert(file.is_open());
      for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
        // console->info("SP_" + std::to_string(sp_id) + "= " +
        //               std::to_string(obj_vals[sp_id]));
        const std::string write_in_file = "SP_" + std::to_string(sp_id) +
                                          " = " +
                                          std::to_string(obj_vals[sp_id]);
        file << write_in_file << '\n';
      }
      file.close();
    }
  }
}
#endif
