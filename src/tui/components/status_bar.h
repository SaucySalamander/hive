#ifndef HIVE_TUI_COMPONENTS_STATUS_BAR_H
#define HIVE_TUI_COMPONENTS_STATUS_BAR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Status bar component for displaying hive metrics.
 * Shows demand buffer depth, waggle strength, active workers, etc.
 */
typedef struct hive_tui_status_bar {
    uint32_t demand_depth;          /* Pending task queue depth */
    uint32_t waggle_strength;       /* Waggle signal strength [0-100] */
    uint32_t active_workers;        /* Number of active workers */
    uint32_t active_alarms;         /* Active alarm count */
    uint32_t quarantined_workers;   /* Quarantined worker count */
    int row;                        /* Row to display on */
} hive_tui_status_bar_t;

/**
 * Initialize a status bar component.
 * @param bar Status bar structure to initialize
 * @param row Row to display on
 */
void hive_tui_status_bar_init(hive_tui_status_bar_t *bar, int row);

/**
 * Draw the status bar on screen.
 */
void hive_tui_status_bar_draw(const hive_tui_status_bar_t *bar);

/**
 * Update status bar metrics.
 * @param bar Status bar structure
 * @param demand_depth Pending task queue depth
 * @param waggle_strength Waggle signal strength [0-100]
 * @param active_workers Number of active workers
 * @param active_alarms Active alarm count
 * @param quarantined_workers Quarantined worker count
 */
void hive_tui_status_bar_update(hive_tui_status_bar_t *bar,
                                uint32_t demand_depth,
                                uint32_t waggle_strength,
                                uint32_t active_workers,
                                uint32_t active_alarms,
                                uint32_t quarantined_workers);

/**
 * Draw a simplified one-liner status bar with key metrics.
 * Used at bottom of pages.
 * @param row Row to display on
 * @param demand_depth Pending tasks
 * @param waggle_strength Signal strength [0-100]
 * @param active_workers Active worker count
 */
void hive_tui_status_bar_draw_oneline(int row, uint32_t demand_depth,
                                      uint32_t waggle_strength,
                                      uint32_t active_workers);

#ifdef __cplusplus
}
#endif

#endif
