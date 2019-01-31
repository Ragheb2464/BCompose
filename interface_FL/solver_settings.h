#ifndef CONTRIB_CONTROL_SETTINGS_H
#define CONTRIB_CONTROL_SETTINGS_H

struct Settings {

  struct StoppingConditions {
    static const uint32_t max_num_iterations = 1000;
    static constexpr const float optimality_gap = 1.0;            // in %
    static constexpr const float root_node_optimality_gap = 0.01; // in %
    static constexpr const float sp_tolerance = 0.1;              // in %
    static constexpr const float branching_time_limit = 3600.0;   // in seconds
    static constexpr const float root_node_time_limit = 3600.0;   // in seconds
    // how much increase in the lb is considered improvement
    static constexpr const float min_lb_improvement = 0.5;
    // for how many iterations the lb is allowed to not improve by
    // min_lb_improvement
    static const uint32_t max_num_non_improvement_iterations = 20;
  };

  struct CutGeneration {
    static const bool use_LR_cuts = false;

    static const bool use_pareto_optimal_cut = true;
    //! AWRNING: Only one of these methods can be true
    static const bool use_MW = true;           // using original method
    static const bool use_modified_MW = false; // using Rahmaniani et's metho
    static const bool use_Sherali_method = false;

    static constexpr const float initial_core_point = 0.5; //
    static const bool smart_core_point_choice = false;     // solve lp to get CP
    static const bool smarter_core_point_choice = false; // solve mip to get CP
    static const bool update_core_point = false;

    static const uint32_t optimize_memory =
        0; // 0: create  one model for each subproblem
           // 1: create one subproblem for each Thread
           // 2: create one model for all subproblems (not applicabale when
           // multi-threaded)
  };

  struct CutAggregation {
    static const uint32_t num_subproblem_cluster =
        0; // 0 means do not aggreegate the subproblems to generate a single cut
  };

  struct Parallelization {
    static const uint32_t num_proc = 7;
  };

  struct GlobalScenarios {
    // See Partial decomposition paper
    static const uint32_t num_retention = 2;
    static const uint32_t num_creation = 1;
    static const uint32_t mood =
        0; // 0 uses random, 1 use heuristic, 2 used mip
  };

  struct CuttingPlane {
    static const bool cutting_plane_for_master = true;
    static const bool cutting_plane_for_subproblems = true;
    static const bool strong_inequalities = false, least_capacity = false,
                      cover_inequalities = false, cardinality_cuts = false,
                      combinatorial_cuts = false, lower_bound_lifting = false,
                      network_connectivity_cut = false, flow_pack = false,
                      flow_cover = false;
  };

  struct WarmStart {
    static const bool run_ws = true;
    static const uint32_t num_ws_iterations = 10;

    static constexpr const float lambda = 0.5;

    static const bool start_from_core_point = true;
  };

  struct Heuristic {
    static const bool run_heuristic = true;
  };

  struct CleanUP {
    static const bool clean_master = true;
  };

  struct CplexSetting {
    static const bool use_default = false;
    //! if default_setting=false, see cplex for def
    static const uint32_t RandomSeed = 2015;
    static const uint32_t ClockType = 1;
    static const uint32_t SimDisplay = 0;
    static const uint32_t AdvInd = 0;
    static const uint32_t Threads = 1;
    static const uint32_t NumericalEmphasis = 1;
    static const uint32_t RootAlgorithm = 1;
    static const uint32_t HeurFreq = -1;
    static const uint32_t Probe = -1;
    static const uint32_t Cliques = -1;
    static const uint32_t Covers = -1;
    static const uint32_t DisjCuts = -1;
    static const uint32_t FlowCovers = -1;
    static const uint32_t FlowPaths = -1;
    static const uint32_t FracCuts = -1;
    static const uint32_t GUBCovers = -1;
    static const uint32_t ImplBd = -1;
    static const uint32_t MCFCuts = -1;
    static const uint32_t MIRCuts = -1;
    static const uint32_t ZeroHalfCuts = -1;
  };

  struct OptimizationModde {
    static const uint32_t mood =
        0; // 0: regular, 1 aggressive (the LB may not be valid)
  };
};

#endif
