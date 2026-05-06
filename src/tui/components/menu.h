#ifndef HIVE_TUI_COMPONENTS_MENU_H
#define HIVE_TUI_COMPONENTS_MENU_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Simple menu selection component for ncurses.
 * Displays a list of items and allows user to navigate/select.
 */
typedef struct hive_tui_menu {
    const char **items;      /* Array of item strings */
    size_t item_count;       /* Number of items */
    size_t selected_index;   /* Currently selected item (0-based) */
    int start_row;           /* Starting row for menu display */
    int start_col;           /* Starting column for menu display */
    int height;              /* Maximum visible items (0 = unlimited) */
} hive_tui_menu_t;

/**
 * Initialize a menu component.
 * @param menu Menu structure to initialize
 * @param items Array of strings for menu items
 * @param count Number of items
 * @param start_row Starting row for display
 * @param start_col Starting column for display
 * @param height Max visible items (0 = unlimited)
 */
void hive_tui_menu_init(hive_tui_menu_t *menu, const char **items, size_t count,
                        int start_row, int start_col, int height);

/**
 * Draw the menu on screen.
 */
void hive_tui_menu_draw(hive_tui_menu_t *menu);

/**
 * Handle user input for menu navigation.
 * Returns the selected index when user presses Enter, -1 for other keys.
 * @param menu Menu structure
 * @param key Input key from getch()
 * @return Selected index on Enter, -1 otherwise
 */
int hive_tui_menu_handle_key(hive_tui_menu_t *menu, int key);

/**
 * Get currently selected item.
 */
const char *hive_tui_menu_get_selected(const hive_tui_menu_t *menu);

#ifdef __cplusplus
}
#endif

#endif
