#ifndef CHARNESS_TOOLS_REGISTRY_H
#define CHARNESS_TOOLS_REGISTRY_H

#include "core/types.h"
#include "logging/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_tool_registry charness_tool_registry_t;

/**
 * Tool kinds exposed to the agent runtime.
 */
typedef enum charness_tool_kind {
    CHARNESS_TOOL_READ_FILE = 0,
    CHARNESS_TOOL_WRITE_FILE,
    CHARNESS_TOOL_RUN_COMMAND,
    CHARNESS_TOOL_LIST_FILES,
    CHARNESS_TOOL_GREP,
    CHARNESS_TOOL_GIT_STATUS,
    CHARNESS_TOOL_KIND_COUNT
} charness_tool_kind_t;

/**
 * Request passed to a tool handler.
 */
typedef struct charness_tool_request {
    charness_tool_kind_t kind;
    const char *path;
    const char *pattern;
    const char *content;
    const char *command;
    bool requires_approval;
} charness_tool_request_t;

/**
 * Result returned by a tool handler.
 */
typedef struct charness_tool_result {
    char *text;
    int exit_code;
} charness_tool_result_t;

/**
 * Tool registry state.
 */
struct charness_tool_registry {
    const char *workspace_root;
    charness_logger_t *logger;
    charness_tool_approval_fn approval_fn;
    void *approval_user_data;
    bool auto_approve;
};

/**
 * Initialize a tool registry.
 *
 * @param registry Registry state.
 * @param workspace_root Root directory used to resolve relative paths.
 * @param logger Optional logger for tool events.
 * @param approval_fn Approval callback.
 * @param approval_user_data Approval callback state.
 * @param auto_approve true to bypass approval prompts.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_tool_registry_init(charness_tool_registry_t *registry,
                                              const char *workspace_root,
                                              charness_logger_t *logger,
                                              charness_tool_approval_fn approval_fn,
                                              void *approval_user_data,
                                              bool auto_approve);

/**
 * Destroy a tool registry.
 *
 * @param registry Registry state.
 */
void charness_tool_registry_deinit(charness_tool_registry_t *registry);

/**
 * Execute one registered tool.
 *
 * @param registry Registry state.
 * @param request Requested tool call.
 * @param result_out Receives the tool output.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_tool_registry_execute(charness_tool_registry_t *registry,
                                                 const charness_tool_request_t *request,
                                                 charness_tool_result_t *result_out);

/**
 * Convert a tool kind to a stable string.
 *
 * @param kind Tool kind.
 * @return Human-readable kind name.
 */
const char *charness_tool_kind_to_string(charness_tool_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
