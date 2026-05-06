#ifndef HIVE_CORE_DYNAMICS_BUFFER_INJECTION_H
#define HIVE_CORE_DYNAMICS_BUFFER_INJECTION_H

#include "dynamics.h"
#include "core/types.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Inject planned work units into the demand buffer.
 *
 * Adds work_units to d->demand_buffer_depth, which will trigger the Queen
 * to emit waggle signals and potentially spawn workers. The waggle signal
 * emission happens naturally during hive_dynamics_tick().
 *
 * This API respects the Queen spawn ratio constraints; workers will spawn
 * as demand_buffer_depth increases per the population regulation logic.
 *
 * @param d          Colony dynamics state.
 * @param work_units Number of work units to inject (added to current depth).
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_dynamics_inject_demand(hive_dynamics_t *d, uint32_t work_units);

/**
 * Get the current demand buffer depth.
 *
 * Returns the current value of d->demand_buffer_depth, which represents
 * pending tasks queued for worker execution.
 *
 * @param d Colony dynamics state.
 * @return Current demand depth (0 if d is NULL).
 */
uint32_t hive_dynamics_get_demand_depth(hive_dynamics_t *d);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_CORE_DYNAMICS_BUFFER_INJECTION_H */
