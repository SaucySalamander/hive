#include "core/agent/agent.h"

#include "common/strings.h"
#include "core/reflexion/reflexion.h"
#include "core/runtime.h"
#include "core/inference/adapter.h"
#include "core/trace/trace.h"
#include "common/logging/logger.h"
/*
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * Include all individual agent headers so that hive_agent_descriptor_for_role()
 * can return the appropriate static descriptor for each colony role.
 */
#include "core/agent/orchestrator.h"
#include "core/agent/planner.h"
#include "core/agent/coder.h"
#include "core/agent/tester.h"
#include "core/agent/verifier.h"
#include "core/agent/editor.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static char *compose_prompt(const char *agent_name,
                            const char *instructions,
                            const hive_runtime_t *runtime,
                            const char *prior_output)
{
    /*
     * Build a chain-of-thought context block from the trace ring.  Pass -1
     * as agent_index so the Queen and every worker see all recent steps
     * (cross-agent reasoning), which lets them learn from peers' outcomes.
     */
    char *cot = hive_trace_build_cot_context(
                    &((hive_runtime_t *)(uintptr_t)runtime)->tracer,
                    -1, 8U);

    char *prompt = hive_string_format(
        "Role: %s\n"
        "Instructions:\n%s\n\n"
        "Workspace root:\n%s\n\n"
        "Project rules:\n%s\n\n"
        "User prompt:\n%s\n\n"
        "Supervisor brief:\n%s\n\n"
        "Prior output:\n%s\n\n"
        "Chain-of-thought context (recent agent trace):\n%s\n",
        safe_text(agent_name),
        safe_text(instructions),
        safe_text(runtime->session.workspace_root),
        safe_text(runtime->session.project_rules),
        safe_text(runtime->session.user_prompt),
        safe_text(runtime->session.orchestrator_brief),
        safe_text(prior_output),
        cot != NULL ? cot : "(no prior trace)");

    free(cot);
    return prompt;
}

hive_status_t hive_agent_generate(hive_runtime_t *runtime,
                                          const char *agent_name,
                                          const char *instructions,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out)
{
    if (runtime == NULL || agent_name == NULL || instructions == NULL || output_out == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    char *prompt = compose_prompt(agent_name, instructions, runtime, prior_output);
    if (prompt == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    hive_inference_request_t request = {
        .agent_name = agent_name,
        .system_prompt = instructions,
        .user_prompt = prompt,
        .context = prior_output,
    };

    if (runtime->logger.initialized) {
        hive_logger_logf(&runtime->logger,
                             HIVE_LOG_DEBUG,
                             "agent",
                             "generate_start",
                             "running %s",
                             safe_text(agent_name));
    }

    hive_inference_response_t response = {0};
    hive_status_t status = hive_inference_adapter_generate(&runtime->adapter, &request, &response);

    if (status != HIVE_STATUS_OK) {
        free(prompt);
        if (runtime->logger.initialized) {
            hive_logger_logf(&runtime->logger,
                                 HIVE_LOG_ERROR,
                                 "agent",
                                 "generate_failed",
                                 "%s failed with %s",
                                 safe_text(agent_name),
                                 hive_status_to_string(status));
        }
        return status;
    }

    char *critique = NULL;
    char *refined = NULL;
    status = hive_reflexion_apply(agent_name, response.text, &critique, &refined);
    free(response.text);
    response.text = NULL;

    if (status != HIVE_STATUS_OK) {
        free(prompt);
        free(critique);
        free(refined);
        return status;
    }

    /* Record the thought process before releasing the prompt buffer. */
    hive_trace_log_thought(&runtime->tracer,
                           runtime->trace_agent_index,
                           agent_name,
                           runtime->trace_stage,
                           prompt,
                           refined,
                           critique);
    free(prompt);
    prompt = NULL;

    if (critique_out != NULL) {
        *critique_out = critique;
    } else {
        free(critique);
    }

    *output_out = refined;

    if (runtime->logger.initialized) {
        hive_logger_logf(&runtime->logger,
                             HIVE_LOG_DEBUG,
                             "agent",
                             "generate_complete",
                             "%s completed",
                             safe_text(agent_name));
    }

    return HIVE_STATUS_OK;
}

hive_status_t hive_agent_run(const hive_agent_t *agent,
                                     hive_runtime_t *runtime,
                                     const char *prior_output,
                                     char **critique_out,
                                     char **output_out)
{
    if (agent == NULL || agent->vtable == NULL || agent->vtable->run == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    return agent->vtable->run(agent, runtime, prior_output, critique_out, output_out);
}

/* ================================================================
 * OPTION 3 IMPLEMENTATION — WORKER-CELL MAPPING
 * Agent descriptor lifecycle helpers.
 * ================================================================ */

hive_agent_t *hive_agent_clone_descriptor(const hive_agent_t *src)
{
    if (src == NULL) return NULL;

    hive_agent_t *clone = calloc(1U, sizeof(*clone));
    if (clone == NULL) return NULL;

    clone->kind   = src->kind;
    clone->vtable = src->vtable;   /* shared; vtables are static, never freed */
    clone->model_override = src->model_override;  /* shared; model strings are static */

    clone->name = hive_string_dup(src->name);
    if (src->name != NULL && clone->name == NULL) {
        free(clone);
        return NULL;
    }

    clone->instructions = hive_string_dup(src->instructions);
    if (src->instructions != NULL && clone->instructions == NULL) {
        free((char *)clone->name);
        free(clone);
        return NULL;
    }

    return clone;
}

void hive_agent_free(hive_agent_t *agent)
{
    if (agent == NULL) return;
    free((char *)agent->name);          /* cast: these are heap duplicates */
    free((char *)agent->instructions);  /* cast: heap duplicate            */
    free(agent);
}

/**
 * Map a colony cell role to the appropriate static agent descriptor.
 *
 * Role → Kind mapping:
 *   QUEEN   → Orchestrator  (meta-task supervision)
 *   CLEANER → Planner       (planning tasks)
 *   NURSE   → Planner       (nurturing / planning)
 *   BUILDER → Coder         (implementation)
 *   GUARD   → Verifier      (auditing / verification)
 *   FORAGER → Tester        (searching for defects)
 *   DRONE   → Editor        (editorial polishing pass)
 *   EMPTY   → NULL
 */
const hive_agent_t *hive_agent_descriptor_for_role(hive_agent_role_t role)
{
    switch (role) {
    case HIVE_ROLE_QUEEN:   return hive_orchestrator_descriptor();
    case HIVE_ROLE_CLEANER: return hive_planner_descriptor();
    case HIVE_ROLE_NURSE:   return hive_planner_descriptor();
    case HIVE_ROLE_BUILDER: return hive_coder_descriptor();
    case HIVE_ROLE_GUARD:   return hive_verifier_descriptor();
    case HIVE_ROLE_FORAGER: return hive_tester_descriptor();
    case HIVE_ROLE_DRONE:   return hive_editor_descriptor();
    default:                return NULL;
    }
}
