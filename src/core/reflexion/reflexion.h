#ifndef HIVE_CORE_REFLEXION_H
#define HIVE_CORE_REFLEXION_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a self-critique and refined result for an agent draft.
 *
 * @param agent_name Agent or stage name.
 * @param draft Draft output produced by the adapter.
 * @param critique_out Receives an allocated self-critique string.
 * @param refined_out Receives the refined output handed to the next layer.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_reflexion_apply(const char *agent_name,
                                           const char *draft,
                                           char **critique_out,
                                           char **refined_out);

#ifdef __cplusplus
}
#endif

#endif
