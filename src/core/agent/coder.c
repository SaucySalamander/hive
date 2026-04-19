#include "core/agent/coder.h"

#include "core/agent/agent.h"
#include "core/runtime.h"

static hive_status_t coder_agent_run(const hive_agent_t *agent,
                                         hive_runtime_t *runtime,
                                         const char *prior_output,
                                         char **critique_out,
                                         char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t coder_vtable = {
    .run = coder_agent_run,
};

static const hive_agent_t coder_agent = {
    .kind = HIVE_AGENT_CODER,
    .name = "Coder",
    .instructions =
        "Draft implementation guidance or code structure that is portable, leak-free, and easy to verify. "
        "Prioritize correctness and clear control flow.",
    .vtable = &coder_vtable,
};

hive_status_t hive_coder_run(hive_runtime_t *runtime,
                                     const char *prior_output,
                                     char **critique_out,
                                     char **output_out)
{
    return hive_agent_run(&coder_agent, runtime, prior_output, critique_out, output_out);
}
