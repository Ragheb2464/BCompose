//
// Created by Rahmaniani, Ragheb on 12/25/20.
//

#ifndef BCOMPOSE_CONTRIB_ML_H
#define BCOMPOSE_CONTRIB_ML_H

#include <cstdlib>

#include "../../externals/util/csv.h"


inline std::unordered_map <std::string, IloNumArray>
ReadMLPredictions(const std::string file_path = "contrib/ML/predicted_values.csv") {

    io::CSVReader<4> in(file_path);
    in.read_header(io::ignore_extra_column, "Reduced_Cost", "y_lp", "score", "pred");

    double Reduced_Cost, y_lp, score, pred;
    uint64_t row_count = 0;

    std::unordered_map <std::string, IloNumArray> ml_res;
    while (in.read_row(Reduced_Cost, y_lp, score, pred)) {
        ml_res["score"].add(score);
        ml_res["pred"].add(pred);
        ++row_count;
    }
    return ml_res;
}

namespace ML {

    inline std::unordered_map <std::string, IloNumArray>
    GetScores(const IloNumArray &y_lp, const IloNumArray &reduced_costs,
              const std::shared_ptr <spdlog::logger> console) {
        console->info("Predicting the values...");
        assert(y_lp.getSize() == reduced_costs.getSize());
        std::string y_str = "";
        std::string rc_str = "";
        for (int i = 0; i < y_lp.getSize(); ++i) {
            y_str += std::to_string(y_lp[i]) + ",";
            rc_str += std::to_string(reduced_costs[i]) + ",";
        }
        console->info(" Scoring...");
        system(("python3 contrib/ML/predict.py " + y_str + " " + rc_str).c_str());
        console->info(" Reading...");
        std::unordered_map <std::string, IloNumArray> ml_res = ReadMLPredictions();
        console->info("Done.");
        return ml_res;
    }


};

#endif //BCOMPOSE_CONTRIB_ML_H
