#include "tui/components/status_bar.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif
#endif

void hive_tui_status_bar_init(hive_tui_status_bar_t *bar, int row)
{
    if (bar == NULL) return;
    bar->demand_depth = 0;
    bar->waggle_strength = 0;
    bar->active_workers = 0;
    bar->active_alarms = 0;
    bar->quarantined_workers = 0;
    bar->row = row;
}

void hive_tui_status_bar_draw(const hive_tui_status_bar_t *bar)
{
    if (bar == NULL) return;

#if HIVE_HAVE_NCURSES
    mvprintw(bar->row, 0, "Demand: %u | Waggle: %u%% | Workers: %u | Alarms: %u | Quarantined: %u",
             bar->demand_depth, bar->waggle_strength, bar->active_workers,
             bar->active_alarms, bar->quarantined_workers);
    refresh();
#endif
}

void hive_tui_status_bar_update(hive_tui_status_bar_t *bar,
                                uint32_t demand_depth,
                                uint32_t waggle_strength,
                                uint32_t active_workers,
                                uint32_t active_alarms,
                                uint32_t quarantined_workers)
{
    if (bar == NULL) return;
    bar->demand_depth = demand_depth;
    bar->waggle_strength = waggle_strength > 100 ? 100 : waggle_strength;
    bar->active_workers = active_workers;
    bar->active_alarms = active_alarms;
    bar->quarantined_workers = quarantined_workers;
}

void hive_tui_status_bar_draw_oneline(int row, uint32_t demand_depth,
                                      uint32_t waggle_strength,
                                      uint32_t active_workers)
{
#if HIVE_HAVE_NCURSES
    uint32_t waggle = waggle_strength > 100 ? 100 : waggle_strength;
    mvprintw(row, 0, "Demand: %u | Waggle: %u%% | Workers: %u",
             demand_depth, waggle, active_workers);
    clrtoeol();
    refresh();
#endif
}
