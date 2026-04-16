#ifndef CHARNESS_CORE_REFLEXION_H
#define CHARNESS_CORE_REFLEXION_H

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
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_reflexion_apply(const char *agent_name,
                                           const char *draft,
                                           char **critique_out,
                                           char **refined_out);

#ifdef __cplusplus
}
#endif

#endif
