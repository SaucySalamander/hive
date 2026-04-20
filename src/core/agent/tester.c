#include "core/agent/tester.h"

#include "core/agent/agent.h"
#include "core/runtime.h"

static hive_status_t tester_agent_run(const hive_agent_t *agent,
                                          hive_runtime_t *runtime,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t tester_vtable = {
    .run = tester_agent_run,
};

static const hive_agent_t tester_agent = {
    .kind = HIVE_AGENT_TESTER,
    .name = "Tester",
    .instructions =
        "Design test cases, validation commands, and failure modes that can falsify the implementation quickly. "
        "Focus on deterministic coverage and security regressions.",
    .vtable = &tester_vtable,
};

hive_status_t hive_tester_run(hive_runtime_t *runtime,
                                      const char *prior_output,
                                      char **critique_out,
                                      char **output_out)
{
    return hive_agent_run(&tester_agent, runtime, prior_output, critique_out, output_out);
}

/* OPTION 3: expose the static descriptor for the scheduler binding table. */
const hive_agent_t *hive_tester_descriptor(void)
{
    return &tester_agent;
}
