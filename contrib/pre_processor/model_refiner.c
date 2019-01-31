

#include <ilcplex/cplex.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "model_refiner.h"

static void free_and_null(char **ptr) {
  if (*ptr != NULL) {
    free(*ptr);
    *ptr = NULL;
  }
} /* END free_and_null */

static int CPXPUBLIC usersolve(CPXCENVptr env, void *cbdata, int wherefrom,
                               void *cbhandle, int *useraction_p) {
  int status = 0;
  int nodecount;
  CPXLPptr nodelp;
  *useraction_p = CPX_CALLBACK_DEFAULT;
  status = CPXgetcallbacknodelp(env, cbdata, wherefrom, &nodelp);
  if (status) goto TERMINATE;
  status = CPXgetcallbackinfo(env, cbdata, wherefrom,
                              CPX_CALLBACK_INFO_NODE_COUNT, &nodecount);
  if (status) goto TERMINATE;
  status = CPXprimopt(env, nodelp);
  // printf("%s\n", (char *)cbhandle);
  status = CPXwriteprob(env, nodelp, (char *)cbhandle, NULL);
  if (!status) *useraction_p = CPX_CALLBACK_SET;
TERMINATE:
  return (status);
} /* END usersolve */

void lifter(const char *model_dir, const int sp_id, const int aggressiveness,
            const float time_lim) {
  int status = 0;
  double objval;
  CPXENVptr env = NULL;
  CPXLPptr lp = NULL;
  /* Initialize the CPLEX environment */
  env = CPXopenCPLEX(&status);
  if (env == NULL) {
    char errmsg[CPXMESSAGEBUFSIZE];
    fprintf(stderr, "Could not open CPLEX environment.\n");
    CPXgeterrorstring(env, status, errmsg);
    fprintf(stderr, "%s", errmsg);
    goto TERMINATE;
  }
  /* Turn on output to the screen */
  status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_OFF);
  if (status) {
    fprintf(stderr, "Failure to turn off screen indicator, error %d.\n",
            status);
    goto TERMINATE;
  }
  /* Create the problem, using the filename as the problem name */
  lp = CPXcreateprob(env, &status, model_dir);
  if (lp == NULL) {
    fprintf(stderr, "Failed to create LP.\n");
    goto TERMINATE;
  }
  /* Now read the file, and copy the data into the created lp */
  status = CPXreadcopyprob(env, lp, model_dir, NULL);
  if (status) {
    fprintf(stderr, "Failed to read and copy the problem data.\n");
    goto TERMINATE;
  }

  status = CPXsetdblparam(env, CPX_PARAM_TILIM, time_lim);
  if (status) goto TERMINATE;
  status = CPXsetsolvecallbackfunc(env, usersolve, (void *)model_dir);
  {
    { /* Let MIP callbacks work on the original model */
      status = CPXsetintparam(env, CPXPARAM_Preprocessing_Linear, 0);
      if (status) goto TERMINATE;
      status =
          CPXsetintparam(env, CPXPARAM_MIP_Strategy_CallbackReducedLP, CPX_OFF);
      if (status) goto TERMINATE;
      status = CPXsetintparam(env, CPXPARAM_Preprocessing_Presolve, CPX_OFF);
      if (status) goto TERMINATE;
      status = CPXsetintparam(env, CPX_PARAM_PREIND, CPX_OFF);
      if (status) goto TERMINATE;
      // status = CPXsetintparam(env, CPXPARAM_MIP_Limits_CutPasses, 0);

      // WARNING: this MUST be off
      status = CPXsetintparam(env, CPXPARAM_MIP_Strategy_HeuristicFreq, -1);
      if (status) goto TERMINATE;
    }
    {
      /*I just want the root node*/
      // improtant, otherwise presolve will change the model
      // status = CPXsetintparam(env, CPX_PARAM_COEREDIND, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_RELAXPREIND, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_REDUCE, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_PREPASS, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_REPEATPRESOLVE, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_AGGIND, CPX_OFF);
      // status = CPXsetintparam(env, CPX_PARAM_BNDSTRENIND, CPX_OFF);
      // status = CPXsetintparam(env, CPXPARAM_MIP_Limits_CutPasses, -1);
      status = CPXsetintparam(env, CPX_PARAM_CLIQUES, aggressiveness);
      status = CPXsetintparam(env, CPX_PARAM_COVERS, aggressiveness);
      status = CPXsetintparam(env, CPX_PARAM_DISJCUTS, aggressiveness);
      status = CPXsetintparam(env, CPXPARAM_MIP_Cuts_LiftProj, aggressiveness);
      int aggressiveness_ = aggressiveness;
      if (aggressiveness_ > 2) {
        aggressiveness_ = 2;
      }
      status = CPXsetintparam(env, CPX_PARAM_FLOWCOVERS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_FLOWPATHS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_FRACCUTS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_GUBCOVERS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_IMPLBD, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_MIRCUTS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_MCFCUTS, aggressiveness_);
      status = CPXsetintparam(env, CPX_PARAM_ZEROHALFCUTS, aggressiveness_);

      status = CPXsetintparam(env, CPXPARAM_MIP_Limits_Nodes, 0);
      if (status) goto TERMINATE;
      /* Turn on traditional search for use with control callbacks */
      status = CPXsetintparam(env, CPXPARAM_Threads, 1);
      if (status) goto TERMINATE;
      status =
          CPXsetintparam(env, CPX_PARAM_MIPSEARCH, CPX_MIPSEARCH_TRADITIONAL);
      if (status) goto TERMINATE;
      status =
          CPXsetintparam(env, CPXPARAM_Emphasis_MIP, CPX_MIPEMPHASIS_BALANCED);
      if (status) goto TERMINATE;
      status = CPXsetintparam(env, CPXPARAM_MIP_Strategy_Search,
                              CPX_MIPSEARCH_TRADITIONAL);
      if (status) goto TERMINATE;
    }
    /* Optimize the problem and obtain solution */
    status = CPXmipopt(env, lp);
    if (status) {
      fprintf(stderr,
              "Failed to pre-process the SP, probably it is not MIP.\n");
      goto TERMINATE;
    }
    status = CPXgetbestobjval(env, lp, &objval);
    if (status) {
      fprintf(stderr, "Failed to obtain objective value.\n");
      goto TERMINATE;
    }
    obj_vals[sp_id] = objval;

  TERMINATE:
    if (lp != NULL) {
      status = CPXfreeprob(env, &lp);
      if (status) {
        fprintf(stderr, "CPXfreeprob failed, error code %d.\n", status);
      }
    }
    if (env != NULL) {
      status = CPXcloseCPLEX(&env);
      if (status) {
        char errmsg[CPXMESSAGEBUFSIZE];
        fprintf(stderr, "Could not close CPLEX environment.\n");
        CPXgeterrorstring(env, status, errmsg);
        fprintf(stderr, "%s", errmsg);
      }
    }
  }
}
