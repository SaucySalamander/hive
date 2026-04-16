#ifndef HIVE_TUI_TUI_H
#define HIVE_TUI_TUI_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Run the ncurses TUI, or return unavailable when ncurses support is absent.
 *
 * @param runtime Runtime state.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_tui_run(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
