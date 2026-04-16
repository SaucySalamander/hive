#ifndef CHARNESS_CORE_RUNTIME_H
#define CHARNESS_CORE_RUNTIME_H

#include "core/session.h"
#include "core/state_machine.h"
#include "core/types.h"
#include "inference/adapter.h"
#include "logging/logger.h"
#include "tools/registry.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Runtime configuration passed in from the CLI or a host application.
 */
typedef struct charness_runtime_options {
    const char *workspace_root;
    const char *user_prompt;
    const char *rules_path;
    const char *api_bind_address;
    unsigned api_port;
    unsigned max_iterations;
    bool auto_approve;
    bool enable_tui;
    bool enable_api;
    bool use_mock_inference;
    bool enable_syslog;
    charness_tool_approval_fn approval_fn;
    void *approval_user_data;
} charness_runtime_options_t;

/**
 * Full runtime state for one execution.
 */
typedef struct charness_runtime {
    charness_runtime_options_t options;
    charness_logger_t logger;
    charness_inference_adapter_t adapter;
    charness_tool_registry_t tools;
    charness_session_t session;
    charness_state_machine_t machine;
} charness_runtime_t;

/**
 * Initialize the runtime, logger, adapter, tool registry, and session state.
 *
 * @param runtime Runtime state to initialize.
 * @param options Runtime options.
 * @param program_name Process name used for logging.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_runtime_init(charness_runtime_t *runtime,
                                        const charness_runtime_options_t *options,
                                        const char *program_name);

/**
 * Tear down all runtime resources.
 *
 * @param runtime Runtime state to destroy.
 */
void charness_runtime_deinit(charness_runtime_t *runtime);

/**
 * Run the hierarchical agent loop.
 *
 * @param runtime Runtime state.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_runtime_run(charness_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
