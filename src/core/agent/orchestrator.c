#include "core/agent/orchestrator.h"

#include "core/agent/agent.h"
#include "core/runtime.h"

static hive_status_t orchestrator_agent_run(const hive_agent_t *agent,
                                                hive_runtime_t *runtime,
                                                const char *prior_output,
                                                char **critique_out,
                                                char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t orchestrator_vtable = {
    .run = orchestrator_agent_run,
};

static const hive_agent_t orchestrator_agent = {
    .kind = HIVE_AGENT_ORCHESTRATOR,
    .name = "Orchestrator",
    .instructions =
        "You are the supervisor. Summarize the mission, maintain the safety gate, and coordinate the worker agents. "
        "Produce a concise but high-signal brief that emphasizes correctness, security, style, and test coverage.",
    .vtable = &orchestrator_vtable,
};

hive_status_t hive_orchestrator_run(hive_runtime_t *runtime,
                                           const char *prior_output,
                                           char **critique_out,
                                           char **output_out)
{
    return hive_agent_run(&orchestrator_agent, runtime, prior_output, critique_out, output_out);
}

/* OPTION 3: expose the static descriptor for the scheduler binding table. */
const hive_agent_t *hive_orchestrator_descriptor(void)
{
    return &orchestrator_agent;
}
