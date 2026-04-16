#ifndef CHARNESS_CORE_AGENT_H
#define CHARNESS_CORE_AGENT_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_runtime charness_runtime_t;
typedef struct charness_agent charness_agent_t;

/**
 * Function table for an agent implementation.
 */
typedef struct charness_agent_vtable {
    charness_status_t (*run)(const charness_agent_t *agent,
                             charness_runtime_t *runtime,
                             const char *prior_output,
                             char **critique_out,
                             char **output_out);
} charness_agent_vtable_t;

/**
 * Agent descriptor used by the hierarchical runtime.
 */
struct charness_agent {
    charness_agent_kind_t kind;
    const char *name;
    const char *instructions;
    const charness_agent_vtable_t *vtable;
};

/**
 * Run a concrete agent using its function table.
 *
 * @param agent Agent descriptor.
 * @param runtime Runtime state.
 * @param prior_output Optional prior stage output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the refined output.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_agent_run(const charness_agent_t *agent,
                                     charness_runtime_t *runtime,
                                     const char *prior_output,
                                     char **critique_out,
                                     char **output_out);

/**
 * Generate an output for a specific agent stage.
 *
 * @param runtime Runtime state.
 * @param agent_name Human-readable agent name.
 * @param instructions Stage-specific instructions.
 * @param prior_output Optional prior output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the refined output.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_agent_generate(charness_runtime_t *runtime,
                                          const char *agent_name,
                                          const char *instructions,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out);

#ifdef __cplusplus
}
#endif

#endif
