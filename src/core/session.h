#ifndef CHARNESS_CORE_SESSION_H
#define CHARNESS_CORE_SESSION_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared session state for a single charness run.
 */
typedef struct charness_session {
    char *workspace_root;
    char *user_prompt;
    char *project_rules;
    char *orchestrator_brief;
    char *plan;
    char *code;
    char *tests;
    char *verification;
    char *edit;
    char *final_output;
    char *last_critique;
    char *last_error;
    charness_score_t last_score;
    unsigned iteration;
} charness_session_t;

/**
 * Initialize a session and duplicate its owned strings.
 *
 * @param session Session state to initialize.
 * @param workspace_root Workspace root string.
 * @param user_prompt User task prompt.
 * @param project_rules Project rules loaded from disk or defaults.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_session_init(charness_session_t *session,
                                        const char *workspace_root,
                                        const char *user_prompt,
                                        const char *project_rules);

/**
 * Destroy a session and release all owned strings.
 *
 * @param session Session state to destroy.
 */
void charness_session_deinit(charness_session_t *session);

#ifdef __cplusplus
}
#endif

#endif
