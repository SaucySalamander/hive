#include "core/reflexion.h"

#include "common/strings.h"

#include <stdlib.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

hive_status_t hive_reflexion_apply(const char *agent_name,
                                           const char *draft,
                                           char **critique_out,
                                           char **refined_out)
{
    if (refined_out == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    const char *safe_agent = safe_text(agent_name);
    const char *safe_draft = safe_text(draft);
    const size_t draft_length = strlen(safe_draft);

    char *critique = hive_string_format(
        "Agent %s self-critique: the draft is %zu characters long and still needs explicit correctness, security, style, and test validation.",
        safe_agent,
        draft_length);
    if (critique == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    char *refined = hive_string_format(
        "%s\n\n[self-critique]\n%s\n",
        safe_draft,
        critique);
    if (refined == NULL) {
        free(critique);
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    if (critique_out != NULL) {
        *critique_out = critique;
    } else {
        free(critique);
    }

    *refined_out = refined;
    return HIVE_STATUS_OK;
}
