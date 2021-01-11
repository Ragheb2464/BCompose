//
// Created by Rahmaniani, Ragheb on 12/25/20.
//

#ifndef BCOMPOSE_CONTRIB_ML_H
#define BCOMPOSE_CONTRIB_ML_H

#include <cstdlib>

#include "../../externals/util/csv.h"

#include "../shared_info/structures.h"


inline std::unordered_map <std::string, std::vector<float>>
ReadMLPredictions(const std::string file_path = "contrib/ML/predicted_values.csv") {

    io::CSVReader<7> in(file_path);
    in.read_header(io::ignore_extra_column, "Dual","y_lp_sum", "RC", "ylp", "score", "pred", "q");

    double dual, ysum,rc, y_lp, score, pred, q;
    uint64_t row_count = 0;

    std::unordered_map <std::string, std::vector<float>> ml_res;
    while (in.read_row(dual, ysum,rc, y_lp, score, pred, q)) {
        ml_res["score"].push_back(score);
        ml_res["pred"].push_back(pred);
        ml_res["q"].push_back(q);
        ++row_count;
    }
    return ml_res;
}

namespace ML {

    inline std::unordered_map <std::string, std::vector<float>>
    RunML(const SharedInfo &shared_info, const std::shared_ptr <spdlog::logger> console) {
        console->info("Predicting the values...");
        assert(shared_info.master_variables_value.getSize() == shared_info.master_vars_reduced_cost.getSize());
        std::string y_str = "";
        std::string rc_str = "";
        std::string lp_str = "";
        std::string d_str = "";
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            y_str += std::to_string(shared_info.master_variables_value[i]) + ",";
            rc_str += std::to_string(shared_info.master_vars_reduced_cost[i]) + ",";
            lp_str += std::to_string(shared_info.master_variables_vals_sum_lp[i]) + ",";
            d_str += std::to_string(shared_info.master_variables_daul_vals_sum_lp[i]) + ",";
        }
        console->info(" Scoring...");
        system(("python3 contrib/ML/predict.py " + y_str + " " + rc_str + " " + lp_str + " " + d_str).c_str());
        console->info(" Predicting...");
        std::unordered_map <std::string, std::vector<float>> ml_res = ReadMLPredictions();
        console->info("Done.");
        return ml_res;
    }

    inline void GetAccuracy(const SharedInfo &shared_info, const std::shared_ptr <spdlog::logger> console) {
        assert(shared_info.ml_res.at("pred").size() == shared_info.master_variables_value.getSize());
        float m = 0;
        float p = 0;
        float p_count = 0;
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            if (std::abs(std::abs(shared_info.ml_res.at("pred")[i] - shared_info.master_variables_value[i])) < 1e-3) {
                ++m;
            }
            if (std::abs(shared_info.ml_res.at("q")[i] - 0.51) > 1e-3) {
                ++p_count;
                if (std::abs(shared_info.ml_res.at("q")[i] - shared_info.master_variables_value[i]) < 1e-3) {
                    ++p;
                }
            }
        }
        console->info("ML:");
        console->info("  -Full fix accuracy was " +
                      std::to_string(100 * m / shared_info.master_variables_value.getSize()) +
                      "%");
        console->info("  -Quantile fixed " +
                      std::to_string(100 * p_count / shared_info.master_variables_value.getSize()) +
                      "% with accuracy of " +
                      std::to_string(100 * p / (p_count + 1e-10)) +
                      "%");
    }

};

#endif //BCOMPOSE_CONTRIB_ML_H
