#ifndef HIVE_TUI_PAGES_BACKLOG_H
#define HIVE_TUI_PAGES_BACKLOG_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Display the backlog page showing pending tasks and model selection.
 * Allows viewing and managing task queue with per-task model overrides.
 *
 * @param runtime Runtime state with session and dynamics
 * @return HIVE_STATUS_OK on success
 */
hive_status_t hive_tui_page_backlog(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
