#ifndef HIVE_TUI_PAGES_SETTINGS_H
#define HIVE_TUI_PAGES_SETTINGS_H

#include "core/types.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Display the settings page for dynamic backend configuration.
 * Allows switching between mock/copilotcli backends and editing
 * the config JSON without restarting the process.
 *
 * Apply is disabled when run_active is true.
 *
 * @param runtime   Runtime state with active inference adapter
 * @param run_active True when a background run is in progress
 * @return HIVE_STATUS_OK on success
 */
hive_status_t hive_tui_page_settings(hive_runtime_t *runtime, bool run_active);

#ifdef __cplusplus
}
#endif

#endif
