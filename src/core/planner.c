#include "core/planner.h"

#include "core/agent.h"
#include "core/runtime.h"

static charness_status_t planner_agent_run(const charness_agent_t *agent,
                                           charness_runtime_t *runtime,
                                           const char *prior_output,
                                           char **critique_out,
                                           char **output_out)
{
    return charness_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const charness_agent_vtable_t planner_vtable = {
    .run = planner_agent_run,
};

static const charness_agent_t planner_agent = {
    .kind = CHARNESS_AGENT_PLANNER,
    .name = "Planner",
    .instructions =
        "Create a hierarchical implementation plan with milestones, risks, validation steps, and explicit safety checks. "
        "Optimize for correctness before speed.",
    .vtable = &planner_vtable,
};

charness_status_t charness_planner_run(charness_runtime_t *runtime,
                                       const char *prior_output,
                                       char **critique_out,
                                       char **output_out)
{
    return charness_agent_run(&planner_agent, runtime, prior_output, critique_out, output_out);
}
