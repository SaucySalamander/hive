#ifndef HIVE_CORE_AGENT_TOOLS_DISPATCH_H
#define HIVE_CORE_AGENT_TOOLS_DISPATCH_H

#include "core/types.h"
#include "core/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Execute a tool request on behalf of an autonomous agent.
 *
 * This is a small, opinionated dispatcher used by agent code to invoke
 * the tool registry without interactive approval. The caller receives
 * ownership of the returned text buffer and must free it.
 */
hive_status_t hive_agent_execute_tool(hive_runtime_t *runtime,
                                      const char *op,
                                      const char *path,
                                      const char *pattern,
                                      const char *command,
                                      const char *content,
                                      char **text_out,
                                      int *exit_code_out);

#ifdef __cplusplus
}
#endif

#endif
