#ifndef HIVE_TOOLS_REGISTRY_H
#define HIVE_TOOLS_REGISTRY_H

#include "core/types.h"
#include "common/logging/logger.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_tool_registry hive_tool_registry_t;

/**
 * Tool kinds exposed to the agent runtime.
 */
typedef enum hive_tool_kind {
    HIVE_TOOL_READ_FILE = 0,
    HIVE_TOOL_WRITE_FILE,
    HIVE_TOOL_RUN_COMMAND,
    HIVE_TOOL_LIST_FILES,
    HIVE_TOOL_GREP,
    HIVE_TOOL_GIT_STATUS,
    HIVE_TOOL_KIND_COUNT
} hive_tool_kind_t;

/**
 * Request passed to a tool handler.
 */
typedef struct hive_tool_request {
    hive_tool_kind_t kind;
    const char *path;
    const char *pattern;
    const char *content;
    const char *command;
    bool requires_approval;
} hive_tool_request_t;

/**
 * Result returned by a tool handler.
 */
typedef struct hive_tool_result {
    char *text;
    int exit_code;
} hive_tool_result_t;

/**
 * Tool registry state.
 */
struct hive_tool_registry {
    const char *workspace_root;
    hive_logger_t *logger;
    hive_tool_approval_fn approval_fn;
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
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_tool_registry_init(hive_tool_registry_t *registry,
                                              const char *workspace_root,
                                              hive_logger_t *logger,
                                              hive_tool_approval_fn approval_fn,
                                              void *approval_user_data,
                                              bool auto_approve);

/**
 * Destroy a tool registry.
 *
 * @param registry Registry state.
 */
void hive_tool_registry_deinit(hive_tool_registry_t *registry);

/**
 * Execute one registered tool.
 *
 * @param registry Registry state.
 * @param request Requested tool call.
 * @param result_out Receives the tool output.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_tool_registry_execute(hive_tool_registry_t *registry,
                                                 const hive_tool_request_t *request,
                                                 hive_tool_result_t *result_out);

/**
 * Convert a tool kind to a stable string.
 *
 * @param kind Tool kind.
 * @return Human-readable kind name.
 */
const char *hive_tool_kind_to_string(hive_tool_kind_t kind);

#ifdef __cplusplus
}
#endif

#endif
