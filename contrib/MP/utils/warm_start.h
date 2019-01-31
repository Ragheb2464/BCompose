#ifndef CONTRIB_MP_UTIL_WS_H
#define CONTRIB_MP_UTIL_WS_H

#include "../../shared_info/structures.h"
#include "../structures.h"

void SimpleWS(const uint64_t iteration, const MasterModel &master_model,
              SharedInfo &shared_info) {
  if (!iteration) {
    const double core_value = Settings::CutGeneration::initial_core_point;
    for (uint64_t var_id = 0;
         var_id < shared_info.master_variables_value.getSize(); ++var_id) {
      //
      if (master_model.master_variables[var_id].getUB() ==
          master_model.master_variables[var_id].getLB()) { // if var is fixed
        shared_info.master_variables_value[var_id] =
            master_model.master_variables[var_id].getLB();
      } else {
        assert(master_model.master_variables[var_id].getUB() >= core_value);
        shared_info.master_variables_value[var_id] = core_value;
      }
    }
  } else {
    const double lambda = Settings::WarmStart::lambda;
    for (uint64_t var_id = 0;
         var_id < shared_info.master_variables_value.getSize(); ++var_id) {
      shared_info.master_variables_value[var_id] =
          lambda * shared_info.master_variables_value[var_id] +
          (1 - lambda) * master_model.cplex.getValue(
                             master_model.master_variables[var_id]);
    }
  }
}
#endif
