#ifndef HIVE_TUI_COMPONENTS_LIST_H
#define HIVE_TUI_COMPONENTS_LIST_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Simple list display component for ncurses.
 * Displays rows of text with optional selection highlighting.
 */
typedef struct hive_tui_list {
    const char **rows;       /* Array of row strings */
    size_t row_count;        /* Number of rows */
    size_t selected_index;   /* Currently selected row (0-based) */
    int start_row;           /* Starting row for display */
    int start_col;           /* Starting column for display */
    int max_rows;            /* Maximum rows to display (0 = unlimited) */
    int row_width;           /* Width of each row (for truncation) */
} hive_tui_list_t;

/**
 * Initialize a list component.
 * @param list List structure to initialize
 * @param rows Array of row strings (copied by reference, not the strings themselves)
 * @param count Number of rows
 * @param start_row Starting row for display
 * @param start_col Starting column for display
 * @param max_rows Maximum rows to display (0 = unlimited)
 * @param row_width Width of each row
 */
void hive_tui_list_init(hive_tui_list_t *list, const char **rows, size_t count,
                        int start_row, int start_col, int max_rows, int row_width);

/**
 * Draw the list on screen.
 */
void hive_tui_list_draw(hive_tui_list_t *list);

/**
 * Handle user input for list navigation.
 * Returns the selected index on Enter, -1 for other keys.
 * @param list List structure
 * @param key Input key from getch()
 * @return Selected index on Enter, -1 otherwise
 */
int hive_tui_list_handle_key(hive_tui_list_t *list, int key);

/**
 * Get currently selected row.
 */
const char *hive_tui_list_get_selected(const hive_tui_list_t *list);

/**
 * Update list rows.
 */
void hive_tui_list_set_rows(hive_tui_list_t *list, const char **rows, size_t count);

/**
 * Clear selection to index 0.
 */
void hive_tui_list_clear_selection(hive_tui_list_t *list);

#ifdef __cplusplus
}
#endif

#endif
