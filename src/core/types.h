#ifndef HIVE_CORE_TYPES_H
#define HIVE_CORE_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Status codes used across the hive runtime.
 */
typedef enum hive_status {
    HIVE_STATUS_OK = 0,
    HIVE_STATUS_ERROR = 1,
    HIVE_STATUS_INVALID_ARGUMENT = 2,
    HIVE_STATUS_OUT_OF_MEMORY = 3,
    HIVE_STATUS_IO_ERROR = 4,
    HIVE_STATUS_UNAVAILABLE = 5,
    HIVE_STATUS_NEEDS_APPROVAL = 6,
    HIVE_STATUS_CANCELLED = 7
} hive_status_t;

/**
 * Dedicated agent roles in the hierarchical loop.
 */
typedef enum hive_agent_kind {
    HIVE_AGENT_ORCHESTRATOR = 0,
    HIVE_AGENT_PLANNER,
    HIVE_AGENT_CODER,
    HIVE_AGENT_TESTER,
    HIVE_AGENT_VERIFIER,
    HIVE_AGENT_EDITOR,
    HIVE_AGENT_KIND_COUNT
} hive_agent_kind_t;

/**
 * Score bundle produced by the evaluator.
 */
typedef struct hive_score {
    unsigned correctness;
    unsigned security;
    unsigned style;
    unsigned test_coverage;
    unsigned overall;
} hive_score_t;

/**
 * Approval callback used by tool execution gates.
 *
 * @param user_data Opaque callback state.
 * @param tool_name Name of the requested tool.
 * @param details Human-readable request summary.
 * @return true to approve execution, false to deny.
 */
typedef bool (*hive_tool_approval_fn)(void *user_data, const char *tool_name, const char *details);

/**
 * Convert a status code to a stable string.
 *
 * @param status Status code.
 * @return Human-readable status name.
 */
const char *hive_status_to_string(hive_status_t status);

/**
 * Convert an agent role to a stable string.
 *
 * @param kind Agent role.
 * @return Human-readable agent role name.
 */
const char *hive_agent_kind_to_string(hive_agent_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
