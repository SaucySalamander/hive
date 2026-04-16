#include "core/coder.h"

#include "core/agent.h"
#include "core/runtime.h"

static charness_status_t coder_agent_run(const charness_agent_t *agent,
                                         charness_runtime_t *runtime,
                                         const char *prior_output,
                                         char **critique_out,
                                         char **output_out)
{
    return charness_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const charness_agent_vtable_t coder_vtable = {
    .run = coder_agent_run,
};

static const charness_agent_t coder_agent = {
    .kind = CHARNESS_AGENT_CODER,
    .name = "Coder",
    .instructions =
        "Draft implementation guidance or code structure that is portable, leak-free, and easy to verify. "
        "Prioritize correctness and clear control flow.",
    .vtable = &coder_vtable,
};

charness_status_t charness_coder_run(charness_runtime_t *runtime,
                                     const char *prior_output,
                                     char **critique_out,
                                     char **output_out)
{
    return charness_agent_run(&coder_agent, runtime, prior_output, critique_out, output_out);
}
