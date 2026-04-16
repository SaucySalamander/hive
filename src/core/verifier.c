#include "core/verifier.h"

#include "core/agent.h"
#include "core/runtime.h"

static charness_status_t verifier_agent_run(const charness_agent_t *agent,
                                            charness_runtime_t *runtime,
                                            const char *prior_output,
                                            char **critique_out,
                                            char **output_out)
{
    return charness_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const charness_agent_vtable_t verifier_vtable = {
    .run = verifier_agent_run,
};

static const charness_agent_t verifier_agent = {
    .kind = CHARNESS_AGENT_VERIFIER,
    .name = "Verifier",
    .instructions =
        "Audit the previous stage for correctness, security, and missing tests. Call out concrete gaps and remediation steps. ",
    .vtable = &verifier_vtable,
};

charness_status_t charness_verifier_run(charness_runtime_t *runtime,
                                        const char *prior_output,
                                        char **critique_out,
                                        char **output_out)
{
    return charness_agent_run(&verifier_agent, runtime, prior_output, critique_out, output_out);
}
