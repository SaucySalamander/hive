#include "tui/pages/backlog.h"

#include "tui/components/menu.h"
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

hive_status_t hive_tui_page_backlog(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

#if !HIVE_HAVE_NCURSES
    return HIVE_STATUS_UNAVAILABLE;
#else
    erase();
    
    /* Header */
    mvprintw(0, 0, "=== hive backlog ===");
    mvprintw(1, 0, "Backlog depth: %u pending tasks", runtime->dynamics.demand_buffer_depth);
    mvprintw(2, 0, "Available models: %zu", runtime->model_config.model_count);
    
    /* Display available models */
    mvprintw(4, 0, "Available models for new tasks:");
    for (size_t i = 0; i < runtime->model_config.model_count && i < 10; i++) {
        const char *model = runtime->model_config.models[i];
        mvprintw(5 + (int)i, 2, "• %s", safe_text(model));
    }
    
    /* Role-based model defaults */
    int row = 16;
    mvprintw(row, 0, "Role-based model defaults:");
    row++;
    for (size_t i = 0; i < runtime->model_config.role_model_count && row < LINES - 3; i++) {
        const hive_role_model_mapping_t *mapping = &runtime->model_config.role_models[i];
        mvprintw(row, 2, "%-15s => %s",
                 safe_text(mapping->role_name),
                 safe_text(mapping->model_name));
        row++;
    }
    
    /* Global default */
    mvprintw(row, 0, "Global default: %s",
             safe_text(runtime->model_config.default_model));
    row++;
    
    /* Instructions */
    mvprintw(LINES - 3, 0, "Tasks will be assigned models based on role defaults.");
    mvprintw(LINES - 2, 0, "Override available via model_config at task creation time.");
    mvprintw(LINES - 1, 0, "Press any key to continue...");
    
    refresh();
    (void)getch();
    
    return HIVE_STATUS_OK;
#endif
}
