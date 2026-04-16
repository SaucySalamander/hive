#include "core/session.h"

#include "common/strings.h"

#include <stdlib.h>
#include <string.h>

static void free_and_null(char **slot)
{
    if (slot != NULL && *slot != NULL) {
        free(*slot);
        *slot = NULL;
    }
}

hive_status_t hive_session_init(hive_session_t *session,
                                        const char *workspace_root,
                                        const char *user_prompt,
                                        const char *project_rules)
{
    if (session == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(session, 0, sizeof(*session));

    session->workspace_root = hive_string_dup(workspace_root);
    session->user_prompt = hive_string_dup(user_prompt);
    session->project_rules = hive_string_dup(project_rules);

    if ((workspace_root != NULL && session->workspace_root == NULL) ||
        (user_prompt != NULL && session->user_prompt == NULL) ||
        (project_rules != NULL && session->project_rules == NULL)) {
        hive_session_deinit(session);
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    return HIVE_STATUS_OK;
}

void hive_session_deinit(hive_session_t *session)
{
    if (session == NULL) {
        return;
    }

    free_and_null(&session->workspace_root);
    free_and_null(&session->user_prompt);
    free_and_null(&session->project_rules);
    free_and_null(&session->orchestrator_brief);
    free_and_null(&session->plan);
    free_and_null(&session->code);
    free_and_null(&session->tests);
    free_and_null(&session->verification);
    free_and_null(&session->edit);
    free_and_null(&session->final_output);
    free_and_null(&session->last_critique);
    free_and_null(&session->last_error);
    memset(&session->last_score, 0, sizeof(session->last_score));
    session->iteration = 0U;
}
