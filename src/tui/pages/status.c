#include "tui/pages/status.h"

#include "core/runtime.h"
#include "core/dynamics/dynamics.h"
#include "core/model_config.h"
#include "core/types.h"
#include "common/logging/logger.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif
#endif

#include <stdio.h>
#include <string.h>

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

hive_status_t hive_tui_page_status(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

#if !HIVE_HAVE_NCURSES
    return HIVE_STATUS_UNAVAILABLE;
#else
    erase();
    
    /* Header */
    mvprintw(0, 0, "=== hive status ===");
    mvprintw(1, 0, "Colony Health and Lifecycle Status");
    
    /* Colony statistics */
    mvprintw(3, 0, "=== Colony Statistics ===");
    mvprintw(4, 0, "Total agents: %u", runtime->dynamics.stats.total_agents);
    mvprintw(5, 0, "Active bound workers: %u", runtime->dynamics.stats.active_bound_workers);
    mvprintw(6, 0, "Suppressed workers: %u", runtime->dynamics.stats.suppressed_workers);
    
    /* Role breakdown */
    mvprintw(8, 0, "=== Role Distribution ===");
    mvprintw(9, 0, "Queens: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_QUEEN]);
    mvprintw(10, 0, "Cleaners: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_CLEANER]);
    mvprintw(11, 0, "Nurses: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_NURSE]);
    mvprintw(12, 0, "Builders: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_BUILDER]);
    mvprintw(13, 0, "Guards: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_GUARD]);
    mvprintw(14, 0, "Foragers: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_FORAGER]);
    mvprintw(15, 0, "Drones: %u", runtime->dynamics.stats.role_counts[HIVE_ROLE_DRONE]);
    
    /* Signal metrics */
    mvprintw(17, 0, "=== Signals & Coordination ===");
    mvprintw(18, 0, "Active pheromones: %u", runtime->dynamics.stats.active_pheromones);
    mvprintw(19, 0, "Active waggles: %u", runtime->dynamics.stats.active_waggles);
    mvprintw(20, 0, "Waggle strength: %u%%", runtime->dynamics.stats.waggle_strength);
    mvprintw(21, 0, "Active alarms: %u", runtime->dynamics.stats.active_alarms);
    
    /* Demand & worker coordination */
    mvprintw(23, 0, "=== Demand & Worker Coordination ===");
    mvprintw(24, 0, "Demand buffer depth: %u pending tasks", runtime->dynamics.demand_buffer_depth);
    mvprintw(25, 0, "Worker spawns: %u", runtime->dynamics.stats.worker_spawns);
    mvprintw(26, 0, "Requeue events: %u (lineage gen: %u)",
             runtime->dynamics.stats.requeen_events_this_run,
             runtime->dynamics.lineage_generation);
    mvprintw(27, 0, "Vitality checksum: 0x%08x",
             runtime->dynamics.vitality_checksum);
    
    /* Model usage */
    mvprintw(29, 0, "=== Model Configuration ===");
    mvprintw(30, 0, "Available models: %zu", runtime->model_config.model_count);
    mvprintw(31, 0, "Default model: %s", safe_text(runtime->model_config.default_model));
    
    /* Footer */
    mvprintw(LINES - 1, 0, "Press any key to continue...");
    
    refresh();
    (void)getch();
    
    return HIVE_STATUS_OK;
#endif
}
