#include "core/agent/tools_dispatch.h"

#include "tools/registry.h"
#include "core/trace/trace.h"
#include "common/logging/logger.h"
#include "common/strings.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

hive_status_t hive_agent_execute_tool(hive_runtime_t *runtime,
                                      const char *op,
                                      const char *path,
                                      const char *pattern,
                                      const char *command,
                                      const char *content,
                                      char **text_out,
                                      int *exit_code_out)
{
    if (runtime == NULL || op == NULL) return HIVE_STATUS_INVALID_ARGUMENT;

    hive_tool_request_t req = {0};
    req.path = path;
    req.pattern = pattern;
    req.command = command;
    req.content = content;
    req.requires_approval = false; /* autonomous workers: no approval */

    if (strcmp(op, "read_file") == 0) {
        req.kind = HIVE_TOOL_READ_FILE;
    } else if (strcmp(op, "write_file") == 0) {
        req.kind = HIVE_TOOL_WRITE_FILE;
    } else if (strcmp(op, "run_command") == 0 || strcmp(op, "run") == 0) {
        req.kind = HIVE_TOOL_RUN_COMMAND;
    } else if (strcmp(op, "list_files") == 0 || strcmp(op, "list") == 0) {
        req.kind = HIVE_TOOL_LIST_FILES;
    } else if (strcmp(op, "grep") == 0) {
        req.kind = HIVE_TOOL_GREP;
    } else if (strcmp(op, "git_status") == 0 || strcmp(op, "git") == 0) {
        req.kind = HIVE_TOOL_GIT_STATUS;
    } else {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    hive_tool_result_t result = {0};
    hive_status_t status = hive_tool_registry_execute(&runtime->tools, &req, &result);

    /* Record the tool call in the trace ring regardless of success. */
    {
        const char *path_or_cmd = path != NULL ? path
                                : command != NULL ? command
                                : pattern != NULL ? pattern
                                : "";
        hive_trace_log_tool(&runtime->tracer,
                            runtime->trace_agent_index,
                            "",   /* agent name not available here; index suffices */
                            runtime->trace_stage,
                            req.kind,
                            path_or_cmd,
                            result.text,
                            result.exit_code);
    }

    if (status != HIVE_STATUS_OK) {
        free(result.text);
        return status;
    }

    if (text_out != NULL) {
        *text_out = result.text; /* ownership transferred */
    } else {
        free(result.text);
    }

    if (exit_code_out != NULL) {
        *exit_code_out = result.exit_code;
    }

    if (runtime->logger.initialized) {
        hive_logger_logf(&runtime->logger,
                         HIVE_LOG_DEBUG,
                         "agent",
                         "tool_call",
                         "op=%s path=%s exit=%d",
                         safe_text(op),
                         safe_text(path),
                         result.exit_code);
    }

    return HIVE_STATUS_OK;
}
