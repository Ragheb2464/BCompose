//
// Created by Rahmaniani, Ragheb on 12/25/20.
//

#ifndef BCOMPOSE_CONTRIB_ML_H
#define BCOMPOSE_CONTRIB_ML_H

#include <cstdlib>
#include <mutex> // std::mutex

#include "../../externals/util/csv.h"

#include "../shared_info/structures.h"
#include "../../solver_settings.h"

/*
 * During LP phase we record, at the end of LP phase we make a prediction then do that for each fractional node
 * that matching our conditions
 */

namespace ML {
    /*!
     * This function records the historical information generated from MP and SPs to be used in the ML
     */
    inline void RecordHistory(SharedInfo &shared_info, const IloNumArray &rc_) {
        //! SP level info for ML
        for (uint64_t sp_id = 0; sp_id < shared_info.num_subproblems; ++sp_id) {
            if (shared_info.retained_subproblem_ids.count(sp_id)) {
                continue;
            }
            //Only for feasible SPs
//            if (shared_info.subproblem_status[sp_id]) {
            for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
                shared_info.dual_sols[i].push_back(shared_info.dual_values[sp_id][i]);
            }
//            }
        }
        //! MP level info for ML
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            shared_info.lp_sols[i].push_back(shared_info.master_variables_value[i]);
            shared_info.rc_sols[i].push_back(rc_[i]);
        }
    }

    inline void
    ExportHistory(const SharedInfo &shared_info, const std::shared_ptr <spdlog::logger> console, const int iter,
                  const bool export_label = false) {
        console->info("Collecting info for ML");
        assert(iter >= _ml_freq);
        std::ofstream outfile;
        if (export_label) {
            outfile.open("contrib/ML/data/trainNew.txt", std::ios_base::app); // append instead of overwrite
        } else {
            outfile.open("contrib/ML/data/prediction.txt"); // when for predication => overwrite
        }
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            std::string lp_str = "";
            std::string d_str = "";
            std::string rc_str = "";
            assert(iter - _ml_freq < shared_info.lp_sols[i].size());
            for (int j = iter - _ml_freq; j < shared_info.lp_sols[i].size(); ++j) {
                lp_str += std::to_string(shared_info.lp_sols[i][j]) + "_";
                d_str += std::to_string(shared_info.dual_sols[i][j]) + "_";
                rc_str += std::to_string(shared_info.rc_sols[i][j]) + "_";
            }
            outfile
                    <<
                    (shared_info.master_variable_obj_coeff[i] - GetIloMin(shared_info.master_variable_obj_coeff)) /
                    (1e-7 + GetIloMax(shared_info.master_variable_obj_coeff) -
                     GetIloMin(shared_info.master_variable_obj_coeff)) << " "
                    << lp_str << " " << d_str << " " << rc_str;
            if (export_label) {
                outfile << " " << shared_info.master_variables_value[i];
            }
            outfile << std::endl;
        }
        console->info("Done.");
    }

    inline std::unordered_map <std::string, std::vector<float>>
    ReadMLPredictions(const std::string file_path = "contrib/ML/data/predicted_values.csv") {

        io::CSVReader<3> in(file_path);
        in.read_header(io::ignore_extra_column, "score", "pred", "q");

        double score, pred, q;
        uint64_t row_count = 0;

        std::unordered_map <std::string, std::vector<float>> ml_res;
        while (in.read_row(score, pred, q)) {
            ml_res["score"].push_back(score);
            ml_res["pred"].push_back(pred);
            ml_res["q"].push_back(q);
            ++row_count;
        }
        return ml_res;
    }


    inline std::unordered_map <std::string, std::vector<float>>
    RunML(const std::shared_ptr <spdlog::logger> console) {
        console->info("Scoring the variables...");
//        console->info(" Scoring...");
        system(("python3 contrib/ML/inference.py contrib/ML/data/prediction.txt"
//                + lp_str + " "
//                + d_str + " "
//                + rc_str + " "
//                + obj_str).c_str()
               ));
        console->info(" Predicting the values...");
        std::unordered_map <std::string, std::vector<float>> ml_res = ReadMLPredictions();
        console->info("Done.");
        return ml_res;
    }

    inline void GetAccuracy(const SharedInfo &shared_info, const std::shared_ptr <spdlog::logger> console) {

        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            std::cout << shared_info.ml_res.at("score")[i] << " => "
                      << shared_info.lp_sols[i][shared_info.lp_sols[i].size() - 1] << " "
                      << shared_info.ml_res.at("pred")[i] << " "
                      << shared_info.master_variables_value[i] << std::endl;
        }


        assert(shared_info.ml_res.at("pred").size() == shared_info.master_variables_value.getSize());
        float pred_matching_count = 0;
        float q_matching_count = 0;
        float q_fixing_count = 0;
        for (int i = 0; i < shared_info.master_variables_value.getSize(); ++i) {
            if (std::abs(shared_info.ml_res.at("pred")[i] - shared_info.master_variables_value[i]) < 1e-3) {
                ++pred_matching_count;
            }
            //# 0.51 means do not fix
            if (std::abs(shared_info.ml_res.at("q")[i] - 0.51) > 1e-3) {
                ++q_fixing_count;
                if (std::abs(shared_info.ml_res.at("q")[i] - shared_info.master_variables_value[i]) < 1e-3) {
                    ++q_matching_count;
                }
            }
        }
        console->info("ML:");
        console->info("  -Prediction accuracy= " +
                      ValToStr(
                              100 * pred_matching_count / (1e-10 + shared_info.master_variables_value.getSize())) +
                      "%");
        console->info("  -Quantile fixed " +
                      ValToStr(100 * q_fixing_count / shared_info.master_variables_value.getSize()) +
                      "% with accuracy of " +
                      ValToStr(100 * q_matching_count / (q_fixing_count + 1e-10)) +
                      "%");
    }

};

#endif //BCOMPOSE_CONTRIB_ML_H
