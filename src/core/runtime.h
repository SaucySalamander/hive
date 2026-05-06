#ifndef HIVE_CORE_RUNTIME_H
#define HIVE_CORE_RUNTIME_H

#include "core/session.h"
#include "core/state_machine.h"
#include "core/types.h"
#include "core/inference/adapter.h"
#include "core/model_config.h"
#include "common/logging/logger.h"
#include "tools/registry.h"

/* OPTION 3 — Worker-Cell Mapping scheduler */
#include "core/dynamics/dynamics.h"
#include "core/scheduler/scheduler.h"
#include "core/trace/trace.h"

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
    hive_model_config_t model_config;  /**< Per-role model selection and defaults. */
    hive_tool_registry_t tools;
    hive_session_t session;
    hive_state_machine_t machine;   /* used when HIVE_LEGACY_SCHEDULER=1 */
    /* OPTION 3 — Worker-Cell Mapping scheduler (used when HIVE_LEGACY_SCHEDULER=0) */
    hive_dynamics_t  dynamics;
    hive_scheduler_t scheduler;
    hive_trace_ring_t tracer;     /**< Append-only ring buffer of thoughts and effects. */

    /* Dispatch context: written by the scheduler before each agent call so
     * that hive_agent_generate() can tag trace entries without changing its
     * own signature. */
    int                trace_agent_index;  /**< Current cell slot; -1 = unknown. */
    hive_agent_stage_t trace_stage;        /**< Current pipeline stage.          */
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
