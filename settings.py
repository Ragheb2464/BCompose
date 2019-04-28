'''
@author   Ragheb Rahmaniani
@email    ragheb.rahmaniani@gmail.com
@version  02.01.0isr
@date     8/10/2018
@brief    These settings allow controlling some of the
          main features of BCompose to tune the performance.
'''

//= == == == == == == == == == == == ==Problem properties
static const bool _is_complete_recourse = false
//
//= == == == == == == == == == == == ==Parallelization
'''
  This setting allows the user to generate cuts in parallel
'''
static const uint32_t _num_worker_processors = 4
/*
    This setting allows the user to solve the master branch - and-bound in
    parallel
*/
static const uint32_t _num_master_processors = 4
/*NOTES
    -Didive number of the processors on your system to balace the workload.
    -BCompose will may dynamically change number of the master or worker
    processors to improve performance.
*/

// == == == == == == == == == == == == = Root Lifter
/*
    This settings allows the user to further tighten the root node before any
    branching happens and after the root node is fully solved.
*/
static const bool _use_root_lifter = false
/*
    Any positive integer. It indicates number of passes allowed to lift the
    cuts.
*/
static const uint32_t _lifter_aggressiveness = 0
/*
This setting is to control the stopping condition of the root lifter
*/
static const float _min_lift_percentage = 0.3
// static const uint32_t max_lifting_iter = 15

// == == == == == == == == == == == == = Improve Formulations
/*
    This setting allows the user to  to improve the representation of the cut
    generation subproblems.
*/
static const bool _improve_SP_representation = false
/*
    0: moderate
    1: agressive
    2: more aggressive
    3: very aggressive
*/
static const uint32_t _improver_aggressiveness = 0

// == == == == == == == == == == == == = Solver

/*
    This setting allows the use to choose the optimizer to solve the problem

    0: Cplex's B & C
    1: IloBenders
    2: BCompose

    NOTE: : for options 0 and 1, num_retention must be equal to number of SPs
        and num_creation = 0
*/
static const uint32_t _solver = 2

// == == == == == == == == == == == == = Partial Decomposition
/*
    Any number from 0 to number of subproblems
*/
static const uint32_t _num_retention = 0
/*
    0 or 1 at this version
*/
static const uint32_t _num_creation = 0
/*
    This setting allows the user to decide how to retain the global
    subproblem(s).

    0: random(default)
    1: maxcost(recommended)
    2: mincost
    3: optimal: : NOTE, option 1 and 2 are only avaiable when SPs are
        pre - processed
*/
static const uint32_t _mood = 1

// == == == == == == == == == == == == = Pareto Cuts

/*
    This settings allows the user to generate Pareto - optimal cuts.
*/
static const double _perturbation_weight = 1e-7
static const float _initial_core_point = 0.5
//
/*
    Allows the user to control adding cuts to the master problem.
*/
static const double _violation_threshold = 1e-2

// == == == == == == == == == == == == = Advanced Cuts
static const bool _gen_combinatorial_cuts =
    false
    // WARNING must trun off if the problem is not binary
static const bool _gen_strengthened_combinatorial_cuts =
    false
    // conditions apply

// == == == == == == == == == == == == = Warm Start
/*
    This setting allows the user to perturb the initial(weak) master
    solutions via a convex combination with the core point
*/
static const bool _run_ws = true
static const uint32_t _num_ws_iterations = 10
/*
    convex combination weight. Any number from [0 - 1].
*/
static const float _alpha = 0.5

// == == == == == == == == == == == == = Cleaner

static const bool _clean_master = false
static const bool _clean_SPs = false
static const uint32_t _cleaner_aggressiveness =
    0
    // 0 conservative, 1 moderate, 2 aggressive, 3 very aggressive,

// == == == == == == == == == == == == = CPLEX settings

// see IBM IloCplex page for definition of each params
static const uint32_t _RandomSeed = 2018
static const uint32_t _ClockType = 2
static const uint32_t _SimDisplay = 0
static const uint32_t _AdvInd = 1
// 0 turn off, 1 default
static const uint32_t _NumericalEmphasis =
    0
    // 0 defualt. 1 turn on(recommended)
static const uint32_t _Threads = 1
// chose of LP solver for SP and MP problems
static const uint32_t _SP_RootAlg = 1
// default 0
static const uint32_t _MP_RootAlg = 0
// cplex heuristics
static const uint32_t _HeurFreq = 0
static const uint32_t _LBHeur = 0
// local branching, 0 off, 1 on
// cplex cuts..I suggest to observe over an instance to see whic one is the
// most effective and then make it more aggressive
static const uint32_t _Cliques = 0
// 1
static const uint32_t _Covers = 0
// 1
static const uint32_t _DisjCuts = 0
// 1
static const uint32_t _FlowCovers = 0
// 1
static const uint32_t _FlowPaths = 0
// 1
static const uint32_t _FracCuts = 0
// 1
static const uint32_t _GUBCovers = 0
// 1
static const uint32_t _ImplBd = 0
// 1
static const uint32_t _LiftProjCuts = 0
// 1
static const uint32_t _MIRCuts = 0
// 1
static const uint32_t _MCFCuts = 0
// 1
static const uint32_t _ZeroHalfCuts = 0
// 1
// for mp
static const uint32_t _MIPEmphasis =
    0
    // 0: default, try 1 or 4 when in heuristic mode
// exploring the B & B tree
static const uint32_t _Probe = 0
// 0 defualt
static const uint32_t _VarSel = 0
// 0 default
static const uint32_t _NodeSel =
    1
    // 1 defualt
    maybe use 3 when in heuristic mode

// == == == == == == == == == == == == = Stopping conditions

static const uint64_t _node_limit =
    9223372036800000000
    // defualt 9223372036800000000
static const uint32_t _max_num_iterations_phase_I =
    1000
    // ++iter, each time a master solution is used to gen cuts,
static const uint32_t _max_num_iterations =
    1000
    // ++iter, each time a master solution is used to gen cuts,

static const float _optimality_gap = 1
// 0.001
// in %
static const float _root_node_optimality_gap = 0.0005
// in %
// static const  float _sp_tolerance = 0.001
// in %

static const float _branching_time_limit = 36000.0
// in seconds
static const float _root_node_time_limit = 7200.0
// in seconds
static const float _subpproblem_time_limit = 600.0
// in seconds
static const float _lifter_time_limit_per_SP = 600.0
// in seconds

static const float _min_lb_improvement =
    0.5
    // how much increase in the lb is considered as improvement
static const uint32_t _max_num_non_improvement_iterations =
    10
    // for how many iterations the lb is allowed to not improve(by
                                                                // min_lb_improvement) before stoping the LP phase

// == == == == == == == == == == == == = Heurstic Options

// this heuristic will be complementary to the convergence
static const uint32_t _run_lagrang_heuristic =
    0
    // number of iterations to run this heuristic

// this heuristic may compromise global convergence
static const bool _run_lagrang_fixer = false
static const float _tolerance = 0.98
// recomm: if the LR does few iter,
    // this value must be closer to one

// This heuristic may compromise the convergence to true optimal sol
static const bool _run_as_heuristic = false
static const float _heur_aggressiveness =
    0.2
    // any number between(0, 0.5), the larger,
        // the more aggressive is the heuristics
static const uint64_t _frequency = 1000
// any_integer_numbr: specifies number
    // of nodes to run the heuristics...
    // 0: only at the root, <= -1 never
static const int _start_node =
    1000
    // after how many nodes activate the heuristic

# endif
