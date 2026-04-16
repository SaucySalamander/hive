#ifndef CHARNESS_TUI_TUI_H
#define CHARNESS_TUI_TUI_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct charness_runtime charness_runtime_t;

/**
 * Run the ncurses TUI, or return unavailable when ncurses support is absent.
 *
 * @param runtime Runtime state.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_tui_run(charness_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif
