
/** @file main.cpp
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates the parallelization framework.
*/
// TODO:  user sanetizers,

#include <chrono>
#include <ilcplex/ilocplex.h>
#include <map>
#include <memory>
#include <set>

#include "externals/docopt/docopt.h"
#include "externals/rss/current_rss.h"
#include "externals/spdlog/spdlog.h"
#include "externals/util/pair_hasher.h"
#include "externals/util/rand.h"
#include "externals/util/type_conversion.h"

#include "./contrib/control/solver_settings.h"
#include "contrib/MP/master.h"
#include "contrib/SP/subproblem.h"
#include "contrib/heuristic/heuristic.h"
#include "contrib/pre_processor/lifter.h"
#include "contrib/shared_info/structures.h"

#include "contrib/analyzer/analyzer.h"

static const char USAGE[] =
    R"(Applies decomposition to solve the input instance.

      Usage:
        ./main  --model_dir MODEL_DIR --current_dir CURRENT_DIR
        ./main (-h | --help)
        ./main --version

      Options:
        -h --help                   Show this screen.
        --version                   Show version.
        --model_dir MODEL_DIR       Absolute path to the model files's folder.
        --current_dir CURRENT_DIR   Absolute path to the current directory.
)";

int main(int argc, char *argv[]) {
  auto console = spdlog::stdout_logger_mt("stdout");

  console->info(" -Parsing the args...");
  const std::map<std::string, docopt::value> args{
      docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "02.01.0isr")};
  const std::string model_directory = args.at("--model_dir").asString();
  const std::string current_directory = args.at("--current_dir").asString();
  if (current_directory[current_directory.size() - 1] == '/' ||
      model_directory[model_directory.size() - 1] == '/') {
    console->error("Directory shouldn't finish with '/'");
    exit(0);
  }
  SharedInfo shared_info{}; // here you can find all the info which is possibly
  // shared among master and subproblems

  console->info(" -Importing master model...");
  std::unique_ptr<Master> MP{new Master()};
  MP->Initializer(console, shared_info, current_directory);

  if (Settings::ImproveFormulations::improve_SP_representation) {
    console->info(" -Pre-processing the models...");
    LiftSPs(current_directory, shared_info, console);
  }

  console->info(" -Importing subproblem models...");
  std::unique_ptr<Subproblem> SP{new Subproblem()};
  SP->Initializer(console, shared_info, current_directory);

  console->info(" -Analyzing the problem...");
  if (Settings::GlobalScenarios::num_creation) {
    AnalyzeSubproblems(console, shared_info);
  }

  MP->FixSafeVariables(console, shared_info);

  console->info(" -Applying PD...");
  MP->PartialDecompsoition(model_directory, console, shared_info,
                           current_directory);

  console->info(" -Initalization took " + std::to_string(MP->GetDuration()) +
                " seconds.");
  MP->SetInitTime();

  if (Settings::Solver::solver == 0) {
    MP->SolveWithCplexBC(console);
  } else if (Settings::Solver::solver == 1) {
    MP->SolveWithCplexBenders(console);
  } else {
    console->info(" -Solving the LP phase...");
    MP->SolveRootNode(console, shared_info, SP);

    if (Settings::Heuristic::run_as_heuristic) {
      console->info(" -Activating the heuristic...");
      MP->RunAsHeuristic(console, shared_info);
    }

    if (Settings::Cleaner::clean_SPs) {
      console->info(" -Cleaning up the SPs");
      SP->Cleaner(console, shared_info);
    }
    if (Settings::Cleaner::clean_master) {
      console->info(" -Cleaning up the master");
      MP->Cleaner(console);
    }

    console->info(" -Creating the parallel branches on master...");
    MP->ParallelPreparer(console, shared_info, 2);

    console->info(" -Switching to MIP MP...");
    MP->ConvertLPtoMIP();
    MP->BranchingPhase(console, shared_info, SP);
  }

  MP->PrintFinalStats(console);
  console->info(" -Optimization terminated successfully!");
  return 0;
} // end main
