#ifndef CHARNESS_CORE_ORCHESTRATOR_H
#define CHARNESS_CORE_ORCHESTRATOR_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_runtime charness_runtime_t;

/**
 * Run the orchestrator agent.
 *
 * @param runtime Runtime state.
 * @param prior_output Optional prior output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the orchestrator output.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_orchestrator_run(charness_runtime_t *runtime,
                                           const char *prior_output,
                                           char **critique_out,
                                           char **output_out);

#ifdef __cplusplus
}
#endif

#endif
