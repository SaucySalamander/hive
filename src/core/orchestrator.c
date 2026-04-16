#include "core/orchestrator.h"

#include "core/agent.h"
#include "core/runtime.h"

static charness_status_t orchestrator_agent_run(const charness_agent_t *agent,
                                                charness_runtime_t *runtime,
                                                const char *prior_output,
                                                char **critique_out,
                                                char **output_out)
{
    return charness_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const charness_agent_vtable_t orchestrator_vtable = {
    .run = orchestrator_agent_run,
};

static const charness_agent_t orchestrator_agent = {
    .kind = CHARNESS_AGENT_ORCHESTRATOR,
    .name = "Orchestrator",
    .instructions =
        "You are the supervisor. Summarize the mission, maintain the safety gate, and coordinate the worker agents. "
        "Produce a concise but high-signal brief that emphasizes correctness, security, style, and test coverage.",
    .vtable = &orchestrator_vtable,
};

charness_status_t charness_orchestrator_run(charness_runtime_t *runtime,
                                           const char *prior_output,
                                           char **critique_out,
                                           char **output_out)
{
    return charness_agent_run(&orchestrator_agent, runtime, prior_output, critique_out, output_out);
}
