#ifndef CONTRIB_SP_UTILS_LEVEL_H
#define CONTRIB_SP_UTILS_LEVEL_H

class Level {
public:
  Level() {}
  ~Level() {}

  void Initializer(const uint64_t var_size) {
    //! Create the model_
    model_ = IloModel(env_);
    cplex_ = IloCplex(model_);
    objective_ = IloMinimize(env_);
    constraints_ = IloRangeArray(env_);
    //! Initiate the Vars and Vals
    dual_vars_ =
        IloNumVarArray(env_, var_size, -IloInfinity, IloInfinity, ILOFLOAT);
    dual_vals_ = IloNumArray(env_, var_size);
    best_dual_vals_ = IloNumArray(env_, var_size);
    model_.add(dual_vars_);
    //! Create obj
    IloExpr objExpr = IloScalProd(
        dual_vars_, dual_vars_) /* - 2 * IloScalProd(dual_vars_, piStar) */;
    objective_.setExpr(objExpr);
    model_.add(objective_);
    objExpr.end();
    //! Set the cplex_
    cplex_.setOut(env_.getNullStream()); // don't print out info on screen
    cplex_.setParam(IloCplex::Param::Threads, 1);
  }

  void UpdateObjective() {
    objective_.setExpr(IloScalProd(dual_vars_, dual_vars_) -
                       2 * IloScalProd(dual_vars_, best_dual_vals_));
  }

  void UpdateConstraints(const double level) {
    for (int i = 0; i < constraints_.getSize(); i++) {
      constraints_[i].setLB(level);
    }
    // std::cout << "TODO: Check the updating is correct" << '\n';
  }

  void AddLevelConstraint(const double cTx, const double level,
                          const IloNumArray &gradient) {
    assert(IloSum(gradient));
    IloExpr expr = cTx + IloScalProd(gradient, dual_vars_) -
                   IloScalProd(gradient, dual_vals_);
    constraints_.add(expr >= level);
    expr.end();
    model_.add(constraints_);
  }

  void Solve() {
    if (cplex_.solve()) {
      cplex_.getValues(dual_vals_, dual_vars_);
      // cplex_.exportModel("proj.lp");
    } else {
      std::cout << " No solution exists for the proj problem. Please check "
                   "the level"
                << std::endl;
      cplex_.exportModel("proj.lp");
      exit(0);
    }
  }

  void SetInitialDualValues(const IloNumArray &dual) {
    assert(dual.getSize() == dual_vals_.getSize());
    for (uint64_t i = 0; i < dual.getSize(); ++i) {
      dual_vals_[i] = dual[i];
      best_dual_vals_[i] = dual[i];
    }
  }
  void GetBestDualValues(IloNumArray &dual) {
    assert(dual.getSize() == best_dual_vals_.getSize());
    for (uint64_t i = 0; i < best_dual_vals_.getSize(); ++i) {
      dual[i] = best_dual_vals_[i];
    }
  }
  void GetDualValues(IloNumArray &dual) {
    assert(dual.getSize() == dual_vals_.getSize());
    for (uint64_t i = 0; i < dual.getSize(); ++i) {
      dual[i] = dual_vals_[i];
    }
  }

  void UpdateBestDualValue(const IloNumArray &dual) {
    assert(dual.getSize() == best_dual_vals_.getSize());
    for (uint64_t i = 0; i < dual.getSize(); ++i) {
      best_dual_vals_[i] = dual[i];
    }
  }
  // void ExportModel() { cplex_.exportModel("p.lp"); }
  void EndEnv() { env_.end(); }

private:
  // the model_ stuff
  IloEnv env_;
  IloModel model_;
  IloCplex cplex_;
  IloRangeArray constraints_;
  IloObjective objective_;
  // variables
  IloNumVarArray dual_vars_;
  IloNumArray dual_vals_;
  IloNumArray best_dual_vals_;
};

#endif
