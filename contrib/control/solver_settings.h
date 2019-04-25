#ifndef CONTRIB_CONTROL_SOLVER_SETTINGS_H
#define CONTRIB_CONTROL_SOLVER_SETTINGS_H

/**
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    These settings allow controlling some of the
          main features of BCompose to tune the performance.
**/

struct Settings {
  struct RootLifter {
    /*
      This settings allows the user to further tighten the root node before any
      branching happens and after the root node is fully solved.
    */
    static const bool use_root_lifter = false;
    /*
      Any positive integer. It indicates number of passes allowed to lift the
      cuts.
    */
    static const uint32_t aggressiveness = 0;
    /*
    This setting is to control the stopping condition of the root lifter
    */
    static constexpr float min_lift_percentage = 0.3;
    // static const uint32_t max_lifting_iter = 15;
  };
  struct ImproveFormulations {
    /*
      This setting allows the user to  to improve the representation of the cut
      generation subproblems.
    */
    static const bool improve_SP_representation = false;
    /*
        0: moderate
        1: agreessive
        2: more aggressive
        3: very aggressive
    */
    static const uint32_t aggressiveness = 0;
  };
  struct Solver {
    /*
      This setting allows the use to choose the optimizer to solve the problem

      0: Cplex's B&C
      1: IloBenders
      2: BCompose

      NOTE:: for 0 and 1, num_retention must be equal to number of SPs
            and num_creation=0
    */
    static const uint32_t solver = 2;
  };
  struct GlobalScenarios {
    /*
      Any number from 0 to number of subproblems
    */
    static const uint32_t num_retention = 0;
    /*
      0 or 1 only at this version
    */
    static const uint32_t num_creation = 0;
    /*
      This setting allows the user to decide how to retain the global
      subproblem(s).

      0: random (default)
      1: maxcost (recommended)
      2: mincost
      3: optimal:: NOTE, option 1 and 2 are only avaiable when SPs are
        pre-processed
    */
    static const uint32_t mood = 1;
  };
  struct ParetoCuts {
    /*
      This settings allows the user to generate Pareto-optimal cuts.
    */
    static constexpr const double perturbation_weight = 1e-7;
    static constexpr const float initial_core_point = 0.5;  //
    /*
      Allows the user to control adding cuts to the master problem.
    */
    static constexpr const double violation_threshold = 1e-2;
  };
  struct WarmStart {
    /*
      This setting allows the user to perturb the initial (weak) master
      solutions via a convex combination with the core point
    */
    static const bool run_ws = true;
    static const uint32_t num_ws_iterations = 10;
    /*
        convex combination weight. Any number from [0-1].
    */
    static constexpr const float alpha = 0.5;
  };
  struct Cleaner {
    static const bool clean_master = true;
    static const bool clean_SPs = true;
    static const uint32_t aggressiveness =
        0;  // 0 conservative, 1 moderate, 2 aggressive, 3 very aggressive,
  };

  struct CplexSetting {
    // see IBM IloCplex page for definition of each params
    static const uint32_t RandomSeed = 2018;
    static const uint32_t ClockType = 2;
    static const uint32_t SimDisplay = 0;
    static const uint32_t AdvInd = 1;  // 0 turn off, 1 default
    static const uint32_t NumericalEmphasis =
        0;  // 0 defualt. 1 turn on (recommended)
    static const uint32_t Threads = 1;
    // chose of LP solver for SP and MP problems
    static const uint32_t SP_RootAlg = 1;  // default 0
    static const uint32_t MP_RootAlg = 0;
    // cplex heuristics
    static const uint32_t HeurFreq = 0;
    static const uint32_t LBHeur = 0;  // local branching, 0 off, 1 on
    // cplex cuts..I suggest to observe over an instance to see whic one is the
    // most effective and then make it more aggressive
    static const uint32_t Cliques = 0;       // 1
    static const uint32_t Covers = 0;        // 1
    static const uint32_t DisjCuts = 0;      // 1
    static const uint32_t FlowCovers = 0;    // 1
    static const uint32_t FlowPaths = 0;     // 1
    static const uint32_t FracCuts = 0;      // 1
    static const uint32_t GUBCovers = 0;     // 1
    static const uint32_t ImplBd = 0;        // 1
    static const uint32_t LiftProjCuts = 0;  // 1
    static const uint32_t MIRCuts = 0;       // 1
    static const uint32_t MCFCuts = 0;       // 1
    static const uint32_t ZeroHalfCuts = 0;  // 1
    // for mp
    static const uint32_t MIPEmphasis =
        0;  // 0:default, try 1 or 4 when in heuristic mode
    // exploring the B&B tree
    static const uint32_t Probe = 0;   // 0 defualt
    static const uint32_t VarSel = 0;  // 0 default;
    static const uint32_t NodeSel =
        1;  // 1 defualt; maybe use 3 when in heuristic mode
  };

  struct Parallelization {
    /*
      This setting allows the user to generate cuts in parallel
    */
    static const uint32_t num_proc = 7;
    /*
      This setting allows the user to solve the master branch-and-bound in
      parallel
    */
    static const uint32_t num_master_procs = 468366449;
  };

  struct StoppingConditions {
    static const size_t node_limit =
        9223372036800000000;  // defualt 9223372036800000000
    static const uint32_t max_num_iterations_phase_I =
        1000;  // ++iter, each time a master solution is used to gen cuts,
    static const uint32_t max_num_iterations =
        1000;  // ++iter, each time a master solution is used to gen cuts,

    static constexpr const float optimality_gap = 0.001;             // in %
    static constexpr const float root_node_optimality_gap = 0.0005;  // in %
    // static constexpr const float sp_tolerance = 0.001;               // in %

    static constexpr const float branching_time_limit = 36000.0;  // in seconds
    static constexpr const float root_node_time_limit = 7200.0;   // in seconds
    static constexpr const float subpproblem_time_limit = 600.0;  // in seconds
    static constexpr const float lifter_time_limit_per_SP =
        600.0;  // in seconds

    static constexpr const float min_lb_improvement =
        0.5;  // how much increase in the lb is considered as improvement
    static const uint32_t max_num_non_improvement_iterations =
        10;  // for how many iterations the lb is allowed to not improve (by
             // min_lb_improvement) before stoping the LP phase
  };

  struct Heuristic {
    // this heuristic will be complementary to the convergence
    static const uint32_t run_lagrang_heuristic =
        0;  // number of iterations to run this heuristic

    // this heuristic may compromise global convergence
    static const bool run_lagrang_fixer = false;
    static constexpr float tolerance =
        0.98;  // recomm: if the LR does few iter,
               // this value must be closer to one

    // This heuristic may compromise the convergence to true optimal sol
    static const bool run_as_heuristic = false;
    static constexpr float aggressiveness =
        0.2;  // any number between (0,0.5), the larger,
              // the more aggressive is the heuristics
    static const int frequency = 1000;  // any_integer_numbr: specifies number
                                        // of nodes to run the heuristics...
                                        // 0: only at the root, <= -1 never
    static const int start_node =
        1000;  // after how many nodes activate the heuristic
  };

  struct CutAggregator {
    //! Under construction
  };
};

struct ProblemSpecificSettings {
  struct AdvancedCuts {
    static const bool gen_combinatorial_cuts =
        false;  // WARNING must trun off if the problem is not binary
    static const bool gen_strengthened_combinatorial_cuts =
        false;  // conditions apply
  };
  struct ProblemProperties {
    static const bool is_complete_recourse = false;  //
  };
};
#endif
