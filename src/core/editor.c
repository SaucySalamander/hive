#include "core/editor.h"

#include "core/agent.h"
#include "core/runtime.h"

static hive_status_t editor_agent_run(const hive_agent_t *agent,
                                          hive_runtime_t *runtime,
                                          const char *prior_output,
                                          char **critique_out,
                                          char **output_out)
{
    return hive_agent_generate(runtime, agent->name, agent->instructions, prior_output, critique_out, output_out);
}

static const hive_agent_vtable_t editor_vtable = {
    .run = editor_agent_run,
};

static const hive_agent_t editor_agent = {
    .kind = HIVE_AGENT_EDITOR,
    .name = "Editor",
    .instructions =
        "Refine the previous draft into the cleanest, most actionable version. Remove duplication, improve clarity, and keep validation explicit.",
    .vtable = &editor_vtable,
};

hive_status_t hive_editor_run(hive_runtime_t *runtime,
                                      const char *prior_output,
                                      char **critique_out,
                                      char **output_out)
{
    return hive_agent_run(&editor_agent, runtime, prior_output, critique_out, output_out);
}
