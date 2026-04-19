#ifndef HIVE_CORE_EDITOR_H
#define HIVE_CORE_EDITOR_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Run the editor agent.
 *
 * @param runtime Runtime state.
 * @param prior_output Optional prior output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the edited output.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_editor_run(hive_runtime_t *runtime,
                                      const char *prior_output,
                                      char **critique_out,
                                      char **output_out);

#ifdef __cplusplus
}
#endif

#endif
