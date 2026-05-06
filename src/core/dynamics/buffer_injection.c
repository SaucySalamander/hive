#include "buffer_injection.h"
#include "dynamics.h"

hive_status_t hive_dynamics_inject_demand(hive_dynamics_t *d, uint32_t work_units)
{
    if (d == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    if (work_units == 0) {
        return HIVE_STATUS_OK;
    }

    /*
     * Inject work units into the demand buffer.
     * The buffer is uint32_t, so overflow wraps naturally (no protection needed).
     * Waggle signal emission happens naturally in hive_dynamics_tick() when
     * demand_buffer_depth > 0, and population regulation will spawn workers
     * as needed based on the spawn demand ratio.
     */
    d->demand_buffer_depth += work_units;

    return HIVE_STATUS_OK;
}

uint32_t hive_dynamics_get_demand_depth(hive_dynamics_t *d)
{
    if (d == NULL) {
        return 0U;
    }
    return d->demand_buffer_depth;
}
