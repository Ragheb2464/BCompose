#ifndef CONTRIB_CONTROL_SOLVER_SETTINGS_H
#define CONTRIB_CONTROL_SOLVER_SETTINGS_H

/**
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    These settings allow controlling some of the
          main features of BCompose to tune its performance.
**/

//=========================Parallelization
/*
  This setting allows the user to:
    * Generate cuts in parallel;
    * Solve the master branch-and-bound in  parallel.

  NOTES:
    - Didive number of the processors on your system to balace the workload.
    - BCompose will may dynamically change number of the master or worker
      processors to improve performance.
    - New CPLEX update (12.8 and 12.9) seem to have some bug when _num_master_processors>1
*/
inline static const uint32_t _num_worker_processors = 11;
inline static const uint32_t _num_master_processors = 1;

// =========================Root Lifter
/*
  This settings allows the user to (further) tighten the root node before any
  branching happens.
  The user is allowed to control the agressivness of this method:
    - 0 light
    - 1 moderate
    - 2 any positive int indicates number of lifting passes

  The use can also control the stopping condition of the root lifter.
*/
inline static const bool _use_root_lifter = false;
inline static const uint32_t _lifter_aggressiveness = 1;
inline static const float _min_lift_percentage = 0.3;  // in percentages

// =========================Improve Formulations
/*
  This setting allows the user to  to improve the representation of the cut
  generation subproblems. Agressivness of this method can be tuned:
      0: moderate
      1: aggressive
      2: more aggressive
      3: very aggressive
*/
inline static const bool _improve_SP_representation = false;
inline static const uint32_t _improver_aggressiveness = 2;

// =========================ML
/*
 *This component allows the use of a pretrained ML model to predict the final values for y vars.
 * Currently only supports the two heuristic modes (i.e., no guarantee of optimality when turned on)
 *
     0: Stops at the root node with the predicted values
     1: Tries to resolves the false predictions iteratively [more accurate results in slightly more time]
 */
inline static const bool _deploy_ml = true;
inline static const bool _ml_strategy = 0;
inline static const int _ml_freq = 10;

// =========================Solver
/*
  This setting allows the use to choose the optimizer to solve the problem:
    0: Cplex's B&C
    1: IloBenders
    2: BCompose
*/
inline static const uint32_t _solver = 2;

// =========================Partial Decomposition
/*
  This setting allows the user to decide how to retain and add global
  subproblem(s) to the master formulation.
      0: random (default)
      1: maxcost (recommended)
      2: mincost
      3: optimal
  NOTES:
    - Strategy 1 and 2 are only avaiable when SPs are pre-processed
    - Number of the retaintions anything from 0 to number of subproblems
    - Number of creation subproblems can only be 0 or 1 at this version
*/
inline static const uint32_t _num_retention = 1;
inline static const uint32_t _num_creation = 0;
inline static const uint32_t _mood = 1;

//=========================Problem properties
/*
  This setting allows to reduce the memory usage.
  NOTES:
    Make sure the warm start is off.
    If the improver is on, the algorithm may require generating feasibility cuts
*/
inline static const bool _is_complete_recourse = false;
// =========================Pareto Cuts
/*
  This settings allows the user to generate Pareto-optimal cuts.
*/
inline static const double _perturbation_weight = 1e-7;
inline static const float _initial_core_point = 0.5;  //

// =========================Warm Start
/*
  This setting allows the user to perturb the initial (weak) master
  solutions via a convex combination with the core point
  NOTES:
    **For complete recourse problems, this setting may cause generating feas
  cuts
    **convex combination weight. Any number from [0-1].
*/
inline static const bool _run_ws = true;
inline static const uint32_t _num_ws_iterations = 10;
inline static const float _alpha = 0.5;

// =========================Cleaner
/*
  Allows the user to mange the cuts added to the master problem.
    0 conservative,
    1 moderate,
    2 aggressive,
    3 very aggressive,
*/
inline static const double _violation_threshold = 1e-3;
inline static const bool _clean_master = false;
inline static const bool _clean_SPs = false;
inline static const uint32_t _cleaner_aggressiveness = 0;

// =========================CPLEX settings
// see IBM IloCplex page for definition of each params
inline static const uint32_t _RandomSeed = 2018;
inline static const uint32_t _ClockType = 2;
inline static const uint32_t _SimDisplay = 0;
inline static const uint32_t _AdvInd = 2;
inline static const uint32_t _NumericalEmphasis = 0;
inline static const uint32_t _SP_RootAlg = 1;  // primal
inline static const uint32_t _MP_RootAlg = 0;
// cplex heuristics
inline static const uint32_t _HeurFreq = 0;
inline static const uint32_t _LBHeur = 0;
// cplex cuts
inline static const uint32_t _Cliques = 0;
inline static const uint32_t _Covers = 0;
inline static const uint32_t _DisjCuts = 0;
inline static const uint32_t _FlowCovers = 0;
inline static const uint32_t _FlowPaths = 0;
inline static const uint32_t _FracCuts = 0;
inline static const uint32_t _GUBCovers = 0;
inline static const uint32_t _ImplBd = 0;
inline static const uint32_t _LiftProjCuts = 0;
inline static const uint32_t _MIRCuts = 0;
inline static const uint32_t _MCFCuts = 0;
inline static const uint32_t _ZeroHalfCuts = 0;
// for mp
inline static const uint32_t _MIPEmphasis = 0;
// exploring the B&B tree
inline static const uint32_t _Probe = 0;
inline static const uint32_t _VarSel = 0;
inline static const uint32_t _NodeSel = 1;

// =========================Stopping conditions
/*
  This setting allows the user control the stopping condition of the algorithm.
*/
inline static const uint64_t _node_limit =
        9223372036800000000;  // defualt 9223372036800000000
inline static const uint32_t _max_num_iterations_phase_I =
        1000;  // ++iter, each time a master solution is used to gen cuts,
inline static const uint32_t _max_num_iterations =
        1000;  // ++iter, each time a master solution is used to gen cuts,

inline static const float _optimality_gap = 0.001;             // in %
inline static const float _root_node_optimality_gap = 0.0005;  // in %
inline static const float _sp_tolerance = 0.001;               // in %

inline static const float _branching_time_limit = 600.0;    // in seconds
inline static const float _root_node_time_limit = 720.0;     // in seconds
inline static const float _subpproblem_time_limit = 60.0;    // in seconds
inline static const float _lifter_time_limit_per_SP = 60.0;  // in seconds

inline static const float _min_lb_improvement =
        0.0005;  // how much increase in the lb is considered as improvement
inline static const uint32_t _max_num_non_improvement_iterations =
        25;  // for how many iterations the lb is allowed to not improve (by
// min_lb_improvement) before stoping the LP phase

// =========================UNDER DEVELOPMENT=============================

// =========================Heurstic Options

// this heuristic will be complementary to the convergence
inline static const uint32_t _run_lagrang_heuristic =
        0;  // number of iterations to run this heuristic

// this heuristic may compromise global convergence
inline static const bool _run_lagrang_fixer = false;
inline static const float _tolerance =
        0.98;  // recomm: if the LR does few iter,
// this value must be closer to one

// This heuristic may compromise the convergence to true optimal sol
inline static const bool _run_as_heuristic = false;
inline static const float _heur_aggressiveness =
        0.2;  // any number between (0,0.5), the larger,
// the more aggressive is the heuristics
inline static const uint64_t _frequency =
        1000;  // any_integer_numbr: specifies number
// of nodes to run the heuristics...
// 0: only at the root, <= -1 never
inline static const int _start_node =
        1000;  // after how many nodes activate the heuristic

// =========================Advanced Cuts
inline static const bool _gen_combinatorial_cuts =
        false;  // WARNING must trun off if the problem is not binary
inline static const bool _gen_strengthened_combinatorial_cuts =
        false;  // conditions apply

#endif
