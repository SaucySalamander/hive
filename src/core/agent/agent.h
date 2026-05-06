#ifndef HIVE_CORE_AGENT_H
#define HIVE_CORE_AGENT_H

#include "core/types.h"
/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * Include dynamics.h so that hive_agent_descriptor_for_role() can accept a
 * hive_agent_role_t.  dynamics.h in turn only forward-declares struct
 * hive_agent (no circular dependency).
 */
#include "core/dynamics/dynamics.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;
typedef struct hive_agent hive_agent_t;

/**
 * Function table for an agent implementation.
 */
typedef struct hive_agent_vtable {
    hive_status_t (*run)(const hive_agent_t *agent,
                             hive_runtime_t *runtime,
                             const char *prior_output,
                             char **critique_out,
                             char **output_out);
} hive_agent_vtable_t;

/**
 * Agent descriptor used by the hierarchical runtime.
 */
struct hive_agent {
    hive_agent_kind_t kind;
    const char *name;
    const char *instructions;
    const hive_agent_vtable_t *vtable;
    /* Optional task-level model override (for per-task model selection) */
    const char *model_override;
};

/**
 * Run a concrete agent using its function table.
 *
 * @param agent Agent descriptor.
 * @param runtime Runtime state.
 * @param prior_output Optional prior stage output used as context.
 * @param critique_out Receives the self-critique text when requested.
 * @param output_out Receives the refined output.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_agent_run(const hive_agent_t *agent,
                                     hive_runtime_t *runtime,
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
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_agent_generate(hive_runtime_t *runtime,
                                          const char *agent_name,
                                          const char *instructions,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out);

/* ----------------------------------------------------------------
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * Agent descriptor lifecycle and colony-role lookup.
 * ---------------------------------------------------------------- */

/**
 * Allocate a heap copy of a static agent descriptor.
 *
 * Duplicates the name and instructions strings; the vtable pointer is shared
 * (vtables are static, never freed).  The caller owns the returned pointer
 * and must release it with hive_agent_free().
 *
 * @param src Source descriptor (typically from hive_<kind>_descriptor()).
 * @return Heap-allocated copy, or NULL on OOM.
 */
hive_agent_t *hive_agent_clone_descriptor(const hive_agent_t *src);

/**
 * Free a descriptor previously allocated by hive_agent_clone_descriptor().
 *
 * @param agent Heap-allocated descriptor.  NULL is safe (no-op).
 */
void hive_agent_free(hive_agent_t *agent);

/**
 * Map a colony role to the corresponding static agent descriptor.
 *
 * Used by hive_queen_spawn() and hive_scheduler_init() to bind the correct
 * agent pipeline to each cell at spawn time.
 *
 * @param role  Colony role (from hive_agent_role_t in dynamics.h).
 * @return Pointer to the static agent descriptor, or NULL if the role maps
 *         to no pipeline (HIVE_ROLE_EMPTY, HIVE_ROLE_DRONE, unknown).
 */
const hive_agent_t *hive_agent_descriptor_for_role(hive_agent_role_t role);

#ifdef __cplusplus
}
#endif

#endif
