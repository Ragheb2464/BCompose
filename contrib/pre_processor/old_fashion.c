
#include <ilcplex/cplex.h>

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

char *export_name;

static int CPXPUBLIC usersolve(CPXCENVptr env, void *cbdata, int wherefrom,
                               void *cbhandle, int *useraction_p);
static void free_and_null(char **ptr), usage(char *progname);

int main(int argc, char *argv[]) {
  int status = 0;
  int solstat;
  double objval;
  double *x = NULL;

  CPXENVptr env = NULL;
  CPXLPptr lp = NULL;

  int j;
  int cur_numcols;
  int wantorig = 1;
  int nameind = 1;

  if (argc != 2) {
    if (argc != 3 || argv[1][0] != '-' || argv[1][1] != 'r') {
      usage(argv[0]);
      goto TERMINATE;
    }
    wantorig = 0;
    nameind = 2;
  }

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
  lp = CPXcreateprob(env, &status, argv[nameind]);
  export_name = argv[nameind];

  if (lp == NULL) {
    fprintf(stderr, "Failed to create LP.\n");
    goto TERMINATE;
  }

  /* Now read the file, and copy the data into the created lp */

  status = CPXreadcopyprob(env, lp, argv[nameind], NULL);
  if (status) {
    fprintf(stderr, "Failed to read and copy the problem data.\n");
    goto TERMINATE;
  }

  /* Set up to use MIP callbacks */
  // status = CPXsetnodecallbackfunc(env, userselectnode, NULL) ||
  //          CPXsetbranchcallbackfunc(env, usersetbranch, NULL) ||
  status = CPXsetsolvecallbackfunc(env, usersolve, NULL);

  if (wantorig) {
    /* Assure linear mappings between the presolved and original
       models */
    status = CPXsetintparam(env, CPXPARAM_Preprocessing_Linear, 0);
    if (status)
      goto TERMINATE;
    /* Let MIP callbacks work on the original model */
    status =
        CPXsetintparam(env, CPXPARAM_MIP_Strategy_CallbackReducedLP, CPX_OFF);
    if (status)
      goto TERMINATE;
  }
  /*I just want the root node*/
  // improtant, otherwise presolve will change the model
  status = CPXsetintparam(env, CPX_PARAM_PREIND, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_BNDSTRENIND, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_COEREDIND, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_RELAXPREIND, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_REDUCE, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_PREPASS, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_REPEATPRESOLVE, CPX_OFF);
  status = CPXsetintparam(env, CPX_PARAM_AGGIND, CPX_OFF);
  // status = CPXsetintparam(env, CPXPARAM_MIP_Limits_CutPasses, -1);

  // status = CPXsetintparam(env, CPX_PARAM_CLIQUES, -1);
  // status = CPXsetintparam(env, CPX_PARAM_COVERS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_DISJCUTS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_FLOWCOVERS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_FLOWPATHS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_FRACCUTS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_GUBCOVERS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_IMPLBD, -1);
  // status = CPXsetintparam(env, CPX_PARAM_MIRCUTS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_MCFCUTS, -1);
  // status = CPXsetintparam(env, CPX_PARAM_ZEROHALFCUTS, -1);
  // status = CPXsetintparam(env, CPXPARAM_MIP_Cuts_LiftProj, -1);

  // WARNING: this MUST be off
  status = CPXsetintparam(env, CPXPARAM_MIP_Strategy_HeuristicFreq, -1);
  if (status)
    goto TERMINATE;

  status = CPXsetintparam(env, CPXPARAM_Preprocessing_Presolve, CPX_OFF);
  if (status)
    goto TERMINATE;
  // status = CPXsetintparam(env, CPXPARAM_MIP_Limits_CutPasses, 0);
  status = CPXsetintparam(env, CPX_PARAM_PREIND, CPX_OFF);
  if (status)
    goto TERMINATE;
  status = CPXsetintparam(env, CPXPARAM_MIP_Limits_Nodes, 0);
  if (status)
    goto TERMINATE;
  /* Set MIP log interval to 1 */
  status = CPXsetintparam(env, CPXPARAM_MIP_Interval, 1);
  if (status)
    goto TERMINATE;
  /* Turn on traditional search for use with control callbacks */
  status = CPXsetintparam(env, CPXPARAM_ScreenOutput, CPX_OFF);
  status = CPXsetintparam(env, CPXPARAM_MIP_Strategy_Search,
                          CPX_MIPSEARCH_TRADITIONAL);
  if (status)
    goto TERMINATE;
  /* Optimize the problem and obtain solution */
  status = CPXmipopt(env, lp);
  if (status) {
    fprintf(stderr, "Failed to optimize MIP.\n");
    goto TERMINATE;
  }

  // solstat = CPXgetstat(env, lp);
  // printf("Solution status %d.\n", solstat);

  // status = CPXgetobjval(env, lp, &objval);
  status = CPXgetbestobjval(env, lp, &objval);
  if (status) {
    fprintf(stderr, "Failed to obtain objective value.\n");
    goto TERMINATE;
  }
  printf("%.10g ", objval);
  // printf("%.10g ", objval);

  cur_numcols = CPXgetnumcols(env, lp);

  /* Allocate space for solution */

  x = (double *)malloc(cur_numcols * sizeof(double));
  if (x == NULL) {
    fprintf(stderr, "No memory for solution values.\n");
    goto TERMINATE;
  }

  status = CPXgetx(env, lp, x, 0, cur_numcols - 1);
  if (status) {
    // fprintf(stderr, "Failed to obtain solution.\n");
    goto TERMINATE;
  }

  /* Write out the solution */

  // for (j = 0; j < cur_numcols; j++) {
  //   if (fabs(x[j]) > 1e-10) {
  //     printf("Column %d:  Value = %17.10g\n", j, x[j]);
  //   }
  // }

TERMINATE:

  /* Free the solution vector */

  free_and_null((char **)&x);

  /* Free the problem as allocated by CPXcreateprob and
     CPXreadcopyprob, if necessary */

  if (lp != NULL) {
    status = CPXfreeprob(env, &lp);
    if (status) {
      fprintf(stderr, "CPXfreeprob failed, error code %d.\n", status);
    }
  }

  /* Free the CPLEX environment, if necessary */

  if (env != NULL) {
    status = CPXcloseCPLEX(&env);

    /* Note that CPXcloseCPLEX produces no output, so the only
       way to see the cause of the error is to use
       CPXgeterrorstring.  For other CPLEX routines, the errors
       will be seen if the CPXPARAM_ScreenOutput parameter is set to
       CPX_ON */

    if (status) {
      char errmsg[CPXMESSAGEBUFSIZE];
      fprintf(stderr, "Could not close CPLEX environment.\n");
      CPXgeterrorstring(env, status, errmsg);
      fprintf(stderr, "%s", errmsg);
    }
  }

  //   { // writing the obj in a file
  //     FILE *f = fopen("contrib/pre_processor/SP_info.txt", "a");
  //     if (f == NULL) {
  //       printf("Error opening f!\n");
  //       exit(1);
  //     }
  //
  //     int c, j = 0;
  // char val;
  //     for (c = 0; c < strlen(export_name); c = c + 1) {
  //       if (export_name[c] >= '0' && export_name[c] <= '9') {
  //         // printf("%c\n", export_name[c]);
  //         j = j + export_name[c] - '0';
  //       }
  //     }
  //     // int i, j = 0;
  //     // for (i = c; i < strlen(export_name); i++) {
  //     //   if (export_name[i] == '.') {
  //     //     break;
  //     //   }
  //     //   j = j + export_name[i];
  //     // }
  //     // char *name = "SP_" + ;
  //     printf("%d\n", j);
  //     fprintf(f, "SP_%d = %.10g\n", j, objval);
  //     fclose(f);
  //   }

  return (objval);

} /* END main */

/* This simple routine frees up the pointer *ptr, and sets *ptr to
   NULL */

static void free_and_null(char **ptr) {
  if (*ptr != NULL) {
    free(*ptr);
    *ptr = NULL;
  }
} /* END free_and_null */

static void usage(char *progname) {
  fprintf(stderr, "Usage: %s [-r] filename\n", progname);
  fprintf(stderr, "  filename   Name of a file, with .mps, .lp, or .sav\n");
  fprintf(stderr, "             extension, and a possible, additional .gz\n");
  fprintf(stderr, "             extension\n");
  fprintf(stderr, "  -r         Indicates that callbacks will refer to the\n");
  fprintf(stderr, "             presolved model\n");
} /* END usage */

static int CPXPUBLIC usersolve(CPXCENVptr env, void *cbdata, int wherefrom,
                               void *cbhandle, int *useraction_p) {
  int status = 0;
  int nodecount;
  CPXLPptr nodelp;

  *useraction_p = CPX_CALLBACK_DEFAULT;

  /* Get pointer to LP subproblem */

  status = CPXgetcallbacknodelp(env, cbdata, wherefrom, &nodelp);
  if (status)
    goto TERMINATE;

  /* Find out what node is being processed */

  status = CPXgetcallbackinfo(env, cbdata, wherefrom,
                              CPX_CALLBACK_INFO_NODE_COUNT, &nodecount);
  if (status)
    goto TERMINATE;

  /* Solve initial node with primal, others with dual */

  if (nodecount < 1)
    status = CPXprimopt(env, nodelp);
  else
    status = CPXdualopt(env, nodelp);
  // status = CPXpreslvwrite(env, nodelp, export_name, NULL);
  status = CPXwriteprob(env, nodelp, export_name, NULL);
  /* If the solve was OK, set return to say optimization has
     been done in callback, otherwise return the CPLEX error
     code */

  if (!status)
    *useraction_p = CPX_CALLBACK_SET;

TERMINATE:

  return (status);

} /* END usersolve */

// static int CPXPUBLIC usersetbranch(CPXCENVptr env, void *cbdata, int
// wherefrom,
//                                    void *cbhandle, int brtype, int sos,
//                                    int nodecnt, int bdcnt, const int
//                                    *nodebeg, const int *indices, const char
//                                    *lu, const double *bd, const double
//                                    *nodeest, int *useraction_p) {
//   int status = 0;
//
//   int j, bestj = -1;
//   int cols;
//   double maxobj = -CPX_INFBOUND;
//   double maxinf = -CPX_INFBOUND;
//   double xj_inf;
//   double xj_lo;
//   double objval;
//   double *x = NULL;
//   double *obj = NULL;
//   int *feas = NULL;
//
//   char varlu[1];
//   double varbd[1];
//   int seqnum1, seqnum2;
//
//   CPXCLPptr lp;
//
//   /* Initialize useraction to indicate no user action taken */
//
//   *useraction_p = CPX_CALLBACK_DEFAULT;
//
//   /* If CPLEX is choosing an SOS branch, take it */
//
//   if (sos >= 0)
//     return (status);
//
//   /* Get pointer to the problem */
//
//   status = CPXgetcallbacklp(env, cbdata, wherefrom, &lp);
//   if (status) {
//     fprintf(stdout, "Can't get LP pointer.\n");
//     goto TERMINATE;
//   }
//
//   cols = CPXgetnumcols(env, lp);
//   if (cols <= 0) {
//     fprintf(stdout, "Can't get number of columns.\n");
//     status = CPXERR_CALLBACK;
//     goto TERMINATE;
//   }
//
//   /* Get solution values and objective coefficients */
//
//   x = (double *)malloc(cols * sizeof(double));
//   obj = (double *)malloc(cols * sizeof(double));
//   feas = (int *)malloc(cols * sizeof(int));
//   if (x == NULL || obj == NULL || feas == NULL) {
//     fprintf(stdout, "Out of memory.");
//     status = CPXERR_CALLBACK;
//     goto TERMINATE;
//   }
//
//   status = CPXgetcallbacknodex(env, cbdata, wherefrom, x, 0, cols - 1);
//   if (status) {
//     fprintf(stdout, "Can't get node solution.");
//     goto TERMINATE;
//   }
//
//   status = CPXgetcallbacknodeobjval(env, cbdata, wherefrom, &objval);
//   if (status) {
//     fprintf(stdout, "Can't get node objective value.");
//     goto TERMINATE;
//   }
//
//   status = CPXgetobj(env, lp, obj, 0, cols - 1);
//   if (status) {
//     fprintf(stdout, "Can't get obj.");
//     goto TERMINATE;
//   }
//
//   status = CPXgetcallbacknodeintfeas(env, cbdata, wherefrom, feas, 0, cols -
//   1); if (status) {
//     fprintf(stdout, "Can't get variable feasible status for node.");
//     goto TERMINATE;
//   }
//
//   /* Branch on var with largest objective coefficient among those
//      with largest infeasibility */
//
//   for (j = 0; j < cols; j++) {
//     if (feas[j] == CPX_INTEGER_INFEASIBLE) {
//       xj_inf = x[j] - floor(x[j]);
//       if (xj_inf > 0.5)
//         xj_inf = 1.0 - xj_inf;
//
//       if (xj_inf >= maxinf && (xj_inf > maxinf || fabs(obj[j]) >= maxobj)) {
//         bestj = j;
//         maxinf = xj_inf;
//         maxobj = fabs(obj[j]);
//       }
//     }
//   }
//
//   /* If there weren't any eligible variables, take default branch */
//
//   if (bestj < 0) {
//     goto TERMINATE;
//   }
//
//   /* Now set up node descriptions */
//
//   xj_lo = floor(x[bestj]);
//
//   /* Up node */
//
//   varlu[0] = 'L';
//   varbd[0] = xj_lo + 1;
//   status = CPXbranchcallbackbranchbds(env, cbdata, wherefrom, 1, &bestj,
//   varlu,
//                                       varbd, objval, NULL, &seqnum1);
//   if (status)
//     goto TERMINATE;
//
//   /* Down node */
//
//   varlu[0] = 'U';
//   varbd[0] = xj_lo;
//
//   status = CPXbranchcallbackbranchbds(env, cbdata, wherefrom, 1, &bestj,
//   varlu,
//                                       varbd, objval, NULL, &seqnum2);
//   if (status)
//     goto TERMINATE;
//
//   /* Set useraction to indicate a user-specified branch */
//
//   *useraction_p = CPX_CALLBACK_SET;
//
// TERMINATE:
//
//   free_and_null((char **)&x);
//   free_and_null((char **)&obj);
//   free_and_null((char **)&feas);
//   return (status);
//
// } /* END usersetbranch */
//
// static int CPXPUBLIC userselectnode(CPXCENVptr env, void *cbdata, int
// wherefrom,
//                                     void *cbhandle, int *nodenum_p,
//                                     int *useraction_p) {
//   int status = 0;
//
//   int thisnode;
//   int nodesleft;
//   int bestnode = 0;
//   int depth;
//   int maxdepth = -1;
//   double siinf;
//   double maxsiinf = 0.0;
//
//   /* Initialize useraction to indicate no user node selection */
//
//   *useraction_p = CPX_CALLBACK_DEFAULT;
//
//   /* Choose the node with the largest sum of infeasibilities among
//      those at the greatest depth */
//
//   status = CPXgetcallbackinfo(env, cbdata, wherefrom,
//                               CPX_CALLBACK_INFO_NODES_LEFT, &nodesleft);
//   if (status)
//     goto TERMINATE;
//
//   for (thisnode = 0; thisnode < nodesleft; thisnode++) {
//     status = CPXgetcallbacknodeinfo(env, cbdata, wherefrom, thisnode,
//                                     CPX_CALLBACK_INFO_NODE_DEPTH, &depth);
//     if (!status) {
//       status = CPXgetcallbacknodeinfo(env, cbdata, wherefrom, thisnode,
//                                       CPX_CALLBACK_INFO_NODE_SIINF, &siinf);
//     }
//     if (status)
//       break;
//
//     if ((depth >= maxdepth) && (depth > maxdepth || siinf > maxsiinf)) {
//       bestnode = thisnode;
//       maxdepth = depth;
//       maxsiinf = siinf;
//     }
//   }
//
//   *nodenum_p = bestnode;
//   *useraction_p = CPX_CALLBACK_SET;
//
// TERMINATE:
//
//   return (status);
//
// } /* END userselectnode */
