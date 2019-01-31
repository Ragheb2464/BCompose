#ifndef COTRIB_PRE_PROCESSOR_MODEL_REFINER_H
#define COTRIB_PRE_PROCESSOR_MODEL_REFINER_H

double *obj_vals;
static void alloc_vec_size(const int size) {
  obj_vals = (double *)malloc(size * sizeof(double));
}
static void free_and_null(char **ptr);
static int CPXPUBLIC usersolve(CPXCENVptr env, void *cbdata, int wherefrom,
                               void *cbhandle, int *useraction_p);
void lifter(const char *model_dir, const int sp_id, const int aggressiveness,
            const float time_lim);

#endif
