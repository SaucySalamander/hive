#include "core/verifier.h"

#include "core/agent.h"
#include "core/runtime.h"

static hive_status_t verifier_agent_run(const hive_agent_t *agent,
                                            hive_runtime_t *runtime,
                                            const char *prior_output,
                                            char **critique_out,
                                            char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t verifier_vtable = {
    .run = verifier_agent_run,
};

static const hive_agent_t verifier_agent = {
    .kind = HIVE_AGENT_VERIFIER,
    .name = "Verifier",
    .instructions =
        "Audit the previous stage for correctness, security, and missing tests. Call out concrete gaps and remediation steps. ",
    .vtable = &verifier_vtable,
};

hive_status_t hive_verifier_run(hive_runtime_t *runtime,
                                        const char *prior_output,
                                        char **critique_out,
                                        char **output_out)
{
    return hive_agent_run(&verifier_agent, runtime, prior_output, critique_out, output_out);
}
