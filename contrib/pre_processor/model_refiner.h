#ifndef COTRIB_PRE_PROCESSOR_MODEL_REFINER_H
#define COTRIB_PRE_PROCESSOR_MODEL_REFINER_H

// static void free_and_null(char **ptr);
static int CPXPUBLIC usersolve(CPXCENVptr env, void *cbdata, int wherefrom,
                               void *cbhandle, int *useraction_p);
void lifter(const char *model_dir, const int sp_id, const int aggressiveness,
            const float time_lim, double *obj_vals);

#endif
