
/** @file main.cpp
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates and export the models that the solver ingest.
*/
// TODO strengthed combinatorial  cuts

#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>

#include <ilcplex/ilocplex.h>
#include "../../externals/docopt/docopt.h"
#include "../../externals/spdlog/spdlog.h"
#include "../../externals/util/pair_hasher.h"

// #include "utils/inst_gen.h"
#include "utils/standard_inst.h"

#include "utils/master_formulation.h"
#include "utils/subproblem_formulation.h"

static const char USAGE[] =
    R"(Exports a .sav model for the master and each subproblem.

      Usage:
        ./main
        ./main (-h | --help)
        ./main --version

      Options:
        -h --help                       Show this screen.
        --version                       Show version.
)";

int main(int argc, char *argv[]) {
  // Thread safe logger
  auto console = spdlog::stdout_logger_mt("stdout");

  console->info(" -Parsing the args...");
  const std::map<std::string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "02.01.0isr");

  console->info(" -Loading the data...");
  const std::string file_path =
      "data/GRID_MCLP_n100_m100000_d1_100_f1_1_s3.dat";
  SetData(file_path);

  console->info(" -Exporting CPLEX models...");
  CreateMasterModel(console);
  CreateSubproblemModels(console);
  console->info(" -Done!");
  return 0;
}  // end main
