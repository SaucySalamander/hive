#include "tui/pages/dashboard.h"

#include "tui/components/status_bar.h"
#include "core/runtime.h"
#include "core/dynamics/dynamics.h"
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

hive_status_t hive_tui_page_dashboard(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

#if !HIVE_HAVE_NCURSES
    return HIVE_STATUS_UNAVAILABLE;
#else
    erase();
    
    /* Header */
    mvprintw(0, 0, "=== hive dashboard ===");
    mvprintw(1, 0, "Workers: %u active", runtime->dynamics.stats.active_bound_workers);
    
    /* Status bar */
    mvprintw(2, 0, "Demand: %u | Waggle: %u%% | Total Agents: %u",
             runtime->dynamics.demand_buffer_depth,
             runtime->dynamics.stats.waggle_strength,
             runtime->dynamics.stats.total_agents);
    
    /* Column headers */
    mvprintw(4, 0, "%-3s %-8s %-15s %-10s %-8s",
             "ID", "Role", "Model", "Status", "Age");
    mvprintw(5, 0, "---+--------+---------------+----------+--------");
    
    /* Worker table */
    int row = 6;
    for (size_t i = 0; i < runtime->dynamics.agent_count && row < LINES - 2; i++) {
        const hive_agent_cell_t *cell = &runtime->dynamics.agents[i];
        
        if (cell->bound_agent == NULL) {
            continue;  /* Skip empty cells */
        }
        
        /* Get role and model */
        const char *role_str = hive_role_to_string(cell->role);
        const char *model = "default";
        
        /* Try to get model from runtime model config based on agent kind */
        if (cell->bound_agent != NULL) {
            hive_agent_kind_t kind = cell->bound_agent->kind;
            model = hive_model_config_get_model_for_kind(&runtime->model_config, kind);
            if (model == NULL) {
                model = runtime->model_config.default_model;
            }
        }
        model = safe_text(model);
        
        /* Status: bound, active, idle, etc. */
        const char *status = "idle";
        if (cell->bound_agent != NULL && cell->conditioned_ok) {
            status = "active";
        } else if (cell->bound_agent != NULL) {
            status = "conditioned";
        }
        
        mvprintw(row, 0, "%-3zu %-8s %-15s %-10s %-8u",
                 i, role_str, model, status, cell->age_ticks);
        
        row++;
    }
    
    /* Footer */
    mvprintw(LINES - 2, 0, "Lifecycle: %u re-queening events | Lineage: %u",
             runtime->dynamics.stats.requeen_events_this_run,
             runtime->dynamics.lineage_generation);
    mvprintw(LINES - 1, 0, "Press any key to continue...");
    
    refresh();
    (void)getch();
    
    return HIVE_STATUS_OK;
#endif
}
