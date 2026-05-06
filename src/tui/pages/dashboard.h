#ifndef HIVE_TUI_PAGES_DASHBOARD_H
#define HIVE_TUI_PAGES_DASHBOARD_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Display the dashboard page showing worker status, models, and demand.
 * Shows: Worker ID | Role | Model | Status | Demand | Waggle
 *
 * @param runtime Runtime state with dynamics and model config
 * @return HIVE_STATUS_OK on success
 */
hive_status_t hive_tui_page_dashboard(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
