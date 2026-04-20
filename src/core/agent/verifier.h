#ifndef HIVE_CORE_VERIFIER_H
#define HIVE_CORE_VERIFIER_H

#include "core/types.h"

/* Forward declaration: avoids pulling in all of agent.h from a child header. */
struct hive_agent;
typedef struct hive_agent hive_agent_t;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Run the verifier agent.
 *
 * @param runtime Runtime state.
 * @param prior_output Optional prior output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the verifier output.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_verifier_run(hive_runtime_t *runtime,
                                        const char *prior_output,
                                        char **critique_out,
                                        char **output_out);

/**
 * OPTION 3: Return a pointer to the static verifier agent descriptor.
 * The returned pointer is valid for the lifetime of the process.
 */
const hive_agent_t *hive_verifier_descriptor(void);

#ifdef __cplusplus
}
#endif

#endif
