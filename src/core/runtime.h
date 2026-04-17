#ifndef HIVE_CORE_RUNTIME_H
#define HIVE_CORE_RUNTIME_H

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
typedef struct hive_runtime_options {
    const char *workspace_root;
    const char *user_prompt;
    const char *rules_path;
    const char *api_bind_address;
    unsigned api_port;
    unsigned max_iterations;
    bool auto_approve;
    bool enable_tui;
    bool enable_raygui;
    bool enable_gtk4;
    bool enable_api;
    bool use_mock_inference;
    bool enable_syslog;
    hive_tool_approval_fn approval_fn;
    void *approval_user_data;
} hive_runtime_options_t;

/**
 * Full runtime state for one execution.
 */
typedef struct hive_runtime {
    hive_runtime_options_t options;
    hive_logger_t logger;
    hive_inference_adapter_t adapter;
    hive_tool_registry_t tools;
    hive_session_t session;
    hive_state_machine_t machine;
} hive_runtime_t;

/**
 * Initialize the runtime, logger, adapter, tool registry, and session state.
 *
 * @param runtime Runtime state to initialize.
 * @param options Runtime options.
 * @param program_name Process name used for logging.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_runtime_init(hive_runtime_t *runtime,
                                        const hive_runtime_options_t *options,
                                        const char *program_name);

/**
 * Tear down all runtime resources.
 *
 * @param runtime Runtime state to destroy.
 */
void hive_runtime_deinit(hive_runtime_t *runtime);

/**
 * Run the hierarchical agent loop.
 *
 * @param runtime Runtime state.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_runtime_run(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
