
/** @file main.cpp
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    This part creates the parallelization framework.
*/

#include <chrono>
#include <iostream>
#include <fstream>
#include <map>
#include <memory>
#include <set>

#include <ilcplex/ilocplex.h>

#include "externals/docopt/docopt.h"
#include "externals/rss/current_rss.h"
#include "externals/spdlog/spdlog.h"
#include "externals/util/pair_hasher.h"
#include "externals/util/rand.h"
#include "externals/util/type_conversion.h"

#include "contrib/ML/ML.h"
#include "contrib/MP/master.h"
#include "contrib/SP/subproblem.h"
#include "contrib/heuristic/heuristic.h"
#include "contrib/pre_processor/lifter.h"
#include "contrib/shared_info/structures.h"
#include "solver_settings.h"

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
    const auto console = spdlog::stdout_logger_mt("stdout");

    console->info("Parsing the args...");
    const std::map<std::string, docopt::value> args{
            docopt::docopt(USAGE, {argv + 1, argv + argc}, true, "02.01.0isr")};
    const std::string model_directory = args.at("--model_dir").asString();
    const std::string current_directory = args.at("--current_dir").asString();
    if (current_directory[current_directory.size() - 1] == '/' ||
        model_directory[model_directory.size() - 1] == '/') {
        console->error("Directory shouldn't end with '/'");
        exit(0);
    }
    SharedInfo shared_info{}; // here you can find all the info which is possibly
    // shared among master and subproblems

    if (std::thread::hardware_concurrency() <
        _num_worker_processors + _num_master_processors) {
        console->warn(
                "This system has at most " +
                std::to_string(std::thread::hardware_concurrency()) +
                " while you want to use " +
                std::to_string(_num_worker_processors + _num_master_processors));
        std::terminate();
    }

    std::unique_ptr<Master> MP = std::make_unique<Master>();
    std::shared_ptr<Subproblem> SP = std::make_shared<Subproblem>();

    console->info("Importing the master model...");
    MP->Initializer(console, shared_info, current_directory);
    assert(shared_info.num_subproblems);
    if (_improve_SP_representation) {
        console->info("Improving the representations...");
        LiftSPs(current_directory, shared_info);
    }
    if (_solver == 2) {
        console->info("Importing subproblem models...");
        SP->Initializer(console, shared_info, current_directory);
    }
    if (_num_creation) {
        console->info("Analyzing the problem...");
        AnalyzeSubproblems(console, shared_info);
    }
    if (_num_retention + _num_creation || _solver == 0 || _solver == 1) {
        console->info("Applying PD...");
        MP->PartialDecompsoition(model_directory, console, shared_info,
                                 current_directory);
    }
    console->info("Initialization took " + std::to_string(MP->GetDuration()) +
                  " seconds.");
    MP->SetInitTime();

    if (_solver == 0) {
        MP->SolveWithCplexBC(console);
    } else if (_solver == 1) {
        MP->SolveWithCplexBenders(console);
    } else if (_solver == 2) {
        console->info("Setting up 1st phase...");
        MP->SolveRootNode(console, shared_info, SP);
        if (_run_as_heuristic) {
            console->info(" -Activating the heuristic...");
            MP->RunAsHeuristic(console, shared_info);
        }
        if (_clean_SPs) {
            console->info(" -Cleaning up the SPs");
            SP->Cleaner(console, shared_info);
        }
        if (_clean_master) {
            console->info(" -Cleaning up the master");
            MP->Cleaner(console);
        }


        if (_deploy_ml) {
            shared_info.master_variables_value_lp.add(shared_info.master_variables_value);
            shared_info.ml_res = ML::RunML(shared_info, console);
        }
        console->info(" -Switching to MIP MP...");
        MP->ConvertLPtoMIP();
        MP->BranchingPhase(console, shared_info, SP);
    } else {
        console->error("Wrong solver is chosen!");
        return 0;
    }


    MP->PrintFinalStats(shared_info, console);

    console->info(" -Optimization terminated successfully!");
    if (MP->GetFinalGap() < 0.1) {
        std::ofstream outfile;
        outfile.open("logNoRootLifter.txt", std::ios_base::app); // append instead of overwrite
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {


            std::string lp_str = "";
            std::string d_str = "";
            std::string rc_str = "";
            for (int j = 0; j < shared_info.lp_sols[i].size(); ++j) {
                lp_str += std::to_string(shared_info.lp_sols[i][j]) + "_";
                d_str += std::to_string(shared_info.dual_sols[i][j]) + "_";
                rc_str += std::to_string(shared_info.rc_sols[i][j]) + "_";
            }
            outfile
                    << MP->GetRootObj() << " " << MP->GetGlobalUB() << " "
                    << (shared_info.master_variable_obj_coeff[i] - GetIloMin(shared_info.master_variable_obj_coeff)) /
                       (1e-7 + GetIloMax(shared_info.master_variable_obj_coeff) -
                        GetIloMin(shared_info.master_variable_obj_coeff)) << " "
                    << lp_str << " " << d_str << " " << rc_str << " "
                    << shared_info.master_variables_value[i] << std::endl;
        }
    }

    return 0;
} // end main
