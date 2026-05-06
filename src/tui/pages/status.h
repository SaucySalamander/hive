#ifndef HIVE_TUI_PAGES_STATUS_H
#define HIVE_TUI_PAGES_STATUS_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Display the status page showing hive lifecycle, signals, and metrics.
 * Shows: lifecycle transitions, alarm signals, quarantined workers, model usage.
 *
 * @param runtime Runtime state with dynamics and scheduler
 * @return HIVE_STATUS_OK on success
 */
hive_status_t hive_tui_page_status(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
