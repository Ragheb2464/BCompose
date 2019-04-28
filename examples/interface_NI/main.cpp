
/** @file main.cpp
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates and export the models that the solver ingest.
*/

#include <chrono>
#include <map>
#include <memory>
#include <set>

#include <ilcplex/ilocplex.h>
#include "../../externals/docopt/docopt.h"
#include "../../externals/spdlog/spdlog.h"
#include "../../externals/util/pair_hasher.h"

#include "utils/data.h"

// #include "utils/cplex_ext.h"
#include "utils/master_formulation.h"
#include "utils/subproblem_formulation.h"

int main(int argc, char *argv[]) {
  // Thread safe logger
  auto console = spdlog::stdout_logger_mt("stdout");

  console->info(" -Loading the data...");
  std::stringstream charvalue, charvalue2;
  charvalue << argv[5];
  charvalue2 << argv[7];
  int bVal, snipnoVal;
  charvalue >> bVal;
  charvalue2 >> snipnoVal;
  auto data = std::make_shared<Data>(bVal, snipnoVal);
  data->readArcGain(argv[1]);
  data->readIntdArc(argv[2]);
  data->readPsi(argv[3]);
  data->readScenario(argv[4]);

  console->info(" -Exporting CPLEX models...");
  CreateMasterModel(data, console);
  CreateSubproblemModels(data, console);
  // console->info(" -Solving the extensive form...");
  // SolveExtForm(data, console);
  console->info(" -Done!");
  return 0;
}  // end main
