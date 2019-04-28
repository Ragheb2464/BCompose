
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

#include "../../solver_settings.h"
#include "utils/data.h"

#include "utils/master_formulation.h"
#include "utils/subproblem_formulation.h"
// #include "utils/cplex_ext.h"

// static const char USAGE[] =
//     R"(Exports a .sav model for the master and each subproblem.
//
//       Usage:
//         ./main --arcgainname GAINNAME --intd_arcname ARCNAME --psiname PSI
//         --scenname SCEN --budget BUDGET --instanceNo INSTNO --snipno SNIPNO
//         ./main (-h | --help)
//         ./main --version
//
//       Options:
//         -h --help                       Show this screen.
//         --version                       Show version.
//         --arcgainname GAINNAME          X
//         --intd_arcname ARCNAME          X
//         --psiname PSI                   X
//         --scenname SCEN                 X
//         --budget BUDGET                 X
//         --instanceNo INSTNO             X
//         --snipno SNIPNO                 X
// )";

int main(int argc, char *argv[]) {
  // Thread safe logger
  auto console = spdlog::stdout_logger_mt("stdout");

  // console->info(" -Parsing the args...");
  // const std::map<std::string, docopt::value> args =
  //     docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "02.01.0isr");

  console->info(" -Loading the data...");
  std::stringstream charvalue, charvalue2, charvalue3, charvalue4;
  charvalue << argv[3];
  charvalue2 << argv[2];
  charvalue3 << argv[4];
  charvalue4 << argv[5];
  int n_sc, n_scC, n_scCap, capID;
  charvalue >> n_sc;
  charvalue2 >> capID;
  charvalue3 >> n_scC;
  charvalue4 >> n_scCap;
  auto data = std::make_shared<Data_S>(n_sc, n_scCap, n_scC);
  data->readData(argv[1]);

  console->info(" -Exporting CPLEX models...");
  CreateMasterModel(data, console);
  CreateSubproblemModels(data, console);
  console->info(" -Done!");
  return 0;
}  // end main
