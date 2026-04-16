#include "core/tester.h"

#include "core/agent.h"
#include "core/runtime.h"

static charness_status_t tester_agent_run(const charness_agent_t *agent,
                                          charness_runtime_t *runtime,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out)
{
    return charness_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const charness_agent_vtable_t tester_vtable = {
    .run = tester_agent_run,
};

static const charness_agent_t tester_agent = {
    .kind = CHARNESS_AGENT_TESTER,
    .name = "Tester",
    .instructions =
        "Design test cases, validation commands, and failure modes that can falsify the implementation quickly. "
        "Focus on deterministic coverage and security regressions.",
    .vtable = &tester_vtable,
};

charness_status_t charness_tester_run(charness_runtime_t *runtime,
                                      const char *prior_output,
                                      char **critique_out,
                                      char **output_out)
{
    return charness_agent_run(&tester_agent, runtime, prior_output, critique_out, output_out);
}
