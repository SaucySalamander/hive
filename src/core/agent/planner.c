#include "core/agent/planner.h"

#include "core/agent/agent.h"
#include "core/runtime.h"

static hive_status_t planner_agent_run(const hive_agent_t *agent,
                                           hive_runtime_t *runtime,
                                           const char *prior_output,
                                           char **critique_out,
                                           char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t planner_vtable = {
    .run = planner_agent_run,
};

static const hive_agent_t planner_agent = {
    .kind = HIVE_AGENT_PLANNER,
    .name = "Planner",
    .instructions =
        "Create a hierarchical implementation plan with milestones, risks, validation steps, and explicit safety checks. "
        "Optimize for correctness before speed.",
    .vtable = &planner_vtable,
};

hive_status_t hive_planner_run(hive_runtime_t *runtime,
                                       const char *prior_output,
                                       char **critique_out,
                                       char **output_out)
{
    return hive_agent_run(&planner_agent, runtime, prior_output, critique_out, output_out);
}
