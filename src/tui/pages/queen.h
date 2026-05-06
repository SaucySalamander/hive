#ifndef HIVE_TUI_PAGES_QUEEN_H
#define HIVE_TUI_PAGES_QUEEN_H

#include "core/runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Plan mode UI context.
 *
 * Tracks the state of the plan mode interface, including the current plan
 * description, model override, and role hints.
 */
typedef struct hive_tui_plan_context {
    char task_description[4096]; /* Large enough for spec-driven prompts */
    char model_override[128];
    char role_hints[256];
    uint32_t pending_demand;
    int edit_field;   /* 0=task spec, 1=model, 2=role hints */
    int task_cursor;  /* byte offset of editing cursor in task_description */
    int task_scroll;  /* first visible visual line in task area */
} hive_tui_plan_context_t;

/**
 * Run the Queen/Plan mode UI page.
 *
 * Displays plan creation, editing, review, and approval interface integrated
 * with the demand buffer and signal system. On approval, injects demand into
 * the hive dynamics and displays real-time worker spawning and waggle signals.
 *
 * @param runtime Runtime state with dynamics and demand injection capability.
 * @return HIVE_STATUS_OK on success or navigation.
 */
hive_status_t hive_tui_page_queen(hive_runtime_t *runtime);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_TUI_PAGES_QUEEN_H */
