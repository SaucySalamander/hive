#include "core/agent.h"

#include "common/strings.h"
#include "core/reflexion.h"
#include "core/runtime.h"
#include "inference/adapter.h"
#include "logging/logger.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static char *compose_prompt(const char *agent_name,
                            const char *instructions,
                            const charness_runtime_t *runtime,
                            const char *prior_output)
{
    return charness_string_format(
        "Role: %s\n"
        "Instructions:\n%s\n\n"
        "Workspace root:\n%s\n\n"
        "Project rules:\n%s\n\n"
        "User prompt:\n%s\n\n"
        "Supervisor brief:\n%s\n\n"
        "Prior output:\n%s\n",
        safe_text(agent_name),
        safe_text(instructions),
        safe_text(runtime->session.workspace_root),
        safe_text(runtime->session.project_rules),
        safe_text(runtime->session.user_prompt),
        safe_text(runtime->session.orchestrator_brief),
        safe_text(prior_output));
}

charness_status_t charness_agent_generate(charness_runtime_t *runtime,
                                          const char *agent_name,
                                          const char *instructions,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out)
{
    if (runtime == NULL || agent_name == NULL || instructions == NULL || output_out == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    char *prompt = compose_prompt(agent_name, instructions, runtime, prior_output);
    if (prompt == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    charness_inference_request_t request = {
        .agent_name = agent_name,
        .system_prompt = instructions,
        .user_prompt = prompt,
        .context = prior_output,
    };

    if (runtime->logger.initialized) {
        charness_logger_logf(&runtime->logger,
                             CHARNESS_LOG_DEBUG,
                             "agent",
                             "generate_start",
                             "running %s",
                             safe_text(agent_name));
    }

    charness_inference_response_t response = {0};
    charness_status_t status = charness_inference_adapter_generate(&runtime->adapter, &request, &response);
    free(prompt);

    if (status != CHARNESS_STATUS_OK) {
        if (runtime->logger.initialized) {
            charness_logger_logf(&runtime->logger,
                                 CHARNESS_LOG_ERROR,
                                 "agent",
                                 "generate_failed",
                                 "%s failed with %s",
                                 safe_text(agent_name),
                                 charness_status_to_string(status));
        }
        return status;
    }

    char *critique = NULL;
    char *refined = NULL;
    status = charness_reflexion_apply(agent_name, response.text, &critique, &refined);
    free(response.text);

    if (status != CHARNESS_STATUS_OK) {
        free(critique);
        free(refined);
        return status;
    }

    if (critique_out != NULL) {
        *critique_out = critique;
    } else {
        free(critique);
    }

    *output_out = refined;

    if (runtime->logger.initialized) {
        charness_logger_logf(&runtime->logger,
                             CHARNESS_LOG_DEBUG,
                             "agent",
                             "generate_complete",
                             "%s completed",
                             safe_text(agent_name));
    }

    return CHARNESS_STATUS_OK;
}

charness_status_t charness_agent_run(const charness_agent_t *agent,
                                     charness_runtime_t *runtime,
                                     const char *prior_output,
                                     char **critique_out,
                                     char **output_out)
{
    if (agent == NULL || agent->vtable == NULL || agent->vtable->run == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    return agent->vtable->run(agent, runtime, prior_output, critique_out, output_out);
}
