#ifndef HIVE_CORE_SESSION_H
#define HIVE_CORE_SESSION_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Shared session state for a single hive run.
 */
typedef struct hive_session {
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
    hive_score_t last_score;
    unsigned iteration;
} hive_session_t;

/**
 * Initialize a session and duplicate its owned strings.
 *
 * @param session Session state to initialize.
 * @param workspace_root Workspace root string.
 * @param user_prompt User task prompt.
 * @param project_rules Project rules loaded from disk or defaults.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_session_init(hive_session_t *session,
                                        const char *workspace_root,
                                        const char *user_prompt,
                                        const char *project_rules);

/**
 * Destroy a session and release all owned strings.
 *
 * @param session Session state to destroy.
 */
void hive_session_deinit(hive_session_t *session);

#ifdef __cplusplus
}
#endif

#endif
