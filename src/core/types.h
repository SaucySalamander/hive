#ifndef CHARNESS_CORE_TYPES_H
#define CHARNESS_CORE_TYPES_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Status codes used across the charness runtime.
 */
typedef enum charness_status {
    CHARNESS_STATUS_OK = 0,
    CHARNESS_STATUS_ERROR = 1,
    CHARNESS_STATUS_INVALID_ARGUMENT = 2,
    CHARNESS_STATUS_OUT_OF_MEMORY = 3,
    CHARNESS_STATUS_IO_ERROR = 4,
    CHARNESS_STATUS_UNAVAILABLE = 5,
    CHARNESS_STATUS_NEEDS_APPROVAL = 6,
    CHARNESS_STATUS_CANCELLED = 7
} charness_status_t;

/**
 * Dedicated agent roles in the hierarchical loop.
 */
typedef enum charness_agent_kind {
    CHARNESS_AGENT_ORCHESTRATOR = 0,
    CHARNESS_AGENT_PLANNER,
    CHARNESS_AGENT_CODER,
    CHARNESS_AGENT_TESTER,
    CHARNESS_AGENT_VERIFIER,
    CHARNESS_AGENT_EDITOR,
    CHARNESS_AGENT_KIND_COUNT
} charness_agent_kind_t;

/**
 * Score bundle produced by the evaluator.
 */
typedef struct charness_score {
    unsigned correctness;
    unsigned security;
    unsigned style;
    unsigned test_coverage;
    unsigned overall;
} charness_score_t;

/**
 * Approval callback used by tool execution gates.
 *
 * @param user_data Opaque callback state.
 * @param tool_name Name of the requested tool.
 * @param details Human-readable request summary.
 * @return true to approve execution, false to deny.
 */
typedef bool (*charness_tool_approval_fn)(void *user_data, const char *tool_name, const char *details);

/**
 * Convert a status code to a stable string.
 *
 * @param status Status code.
 * @return Human-readable status name.
 */
const char *charness_status_to_string(charness_status_t status);

/**
 * Convert an agent role to a stable string.
 *
 * @param kind Agent role.
 * @return Human-readable agent role name.
 */
const char *charness_agent_kind_to_string(charness_agent_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
