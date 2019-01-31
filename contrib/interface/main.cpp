
/** @file main.cpp
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates the parallelization framework.
*/

#include <chrono>
#include <map>
#include <memory>
#include <set>

#include "../externals/docopt/docopt.h"
#include "../externals/spdlog/spdlog.h"
#include "../externals/util/pair_hasher.h"
#include <ilcplex/ilocplex.h>

#include "../contrib/control/data.h"

#include "settings.h"

#include "master_formulation.h"
#include "subproblem_formulation.h"

static const char USAGE[] =
    R"(Applies decomposition to solve the input instance.

      Usage:
        ./main --num_nodes NODES --num_od OD --num_arcs ARCS --num_scenario SCENARIO --file_path FILE_PATH --scenario_path SCENARIO_PARTH
        ./main (-h | --help)
        ./main --version

      Options:
        -h --help                       Show this screen.
        --version                       Show version.
        --num_nodes NODES               Specifies number of nodes in the graph.
        --num_od OD                     Specifies number of origin-destination pairs.
        --num_arcs ARCS                 Specifies number of arcs in the graph.
        --num_scenario SCENARIO         Gives number of scenarios.
        --file_path FILE_PATH           Path to network file.
        --scenario_path SCENARIO_PARTH  Path to scenario file.
)";
// using namespace std;
int main(int argc, char *argv[]) {
  // Thread safe logger
  auto console = spdlog::stdout_logger_mt("stdout");

  console->info(" -Parsing the args...");
  const std::map<std::string, docopt::value> args =
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "02.01.0isr");

  console->info(" -Loading the data...");
  Data data;
  data.num_scenario = std::stoi(args.at("--num_scenario").asString());
  data.num_nodes = std::stoi(args.at("--num_nodes").asString());
  data.num_arcs = std::stoi(args.at("--num_arcs").asString());
  data.num_od = std::stoi(args.at("--num_od").asString());
  GraphFileReader(args.at("--file_path").asString(), data, console);
  ScenarioFileReader(args.at("--scenario_path").asString(), data, console);

  console->info(" -Exporting CPLEX models...");
  CreateMasterModel(data, console);
  CreateSubproblemModels(data, console);

  console->info(" -Done!");
  return 0;
} // end main
