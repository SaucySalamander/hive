#include "tui/components/list.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif
#endif

#include <string.h>

void hive_tui_list_init(hive_tui_list_t *list, const char **rows, size_t count,
                        int start_row, int start_col, int max_rows, int row_width)
{
    if (list == NULL) return;
    list->rows = rows;
    list->row_count = count;
    list->selected_index = 0;
    list->start_row = start_row;
    list->start_col = start_col;
    list->max_rows = max_rows;
    list->row_width = row_width > 0 ? row_width : 80;
}

void hive_tui_list_draw(hive_tui_list_t *list)
{
    if (list == NULL || list->rows == NULL) return;

#if HIVE_HAVE_NCURSES
    int row = list->start_row;
    int display_rows = list->max_rows > 0 ? list->max_rows : (int)list->row_count;
    int start_item = list->selected_index > (size_t)display_rows ?
                     (int)list->selected_index - display_rows + 1 : 0;

    for (int i = 0; i < display_rows && start_item + i < (int)list->row_count; i++) {
        const char *item = list->rows[start_item + i];
        int attr = (start_item + i == (int)list->selected_index) ?
                   A_REVERSE : A_NORMAL;
        
        attron(attr);
        mvprintw(row + i, list->start_col, "%-*s", list->row_width,
                 item ? item : "");
        attroff(attr);
    }
    refresh();
#endif
}

int hive_tui_list_handle_key(hive_tui_list_t *list, int key)
{
    if (list == NULL) return -1;

    switch (key) {
        case KEY_UP:
            if (list->selected_index > 0) list->selected_index--;
            else if (list->row_count > 0) list->selected_index = list->row_count - 1;
            return -1;
        case KEY_DOWN:
            if (list->selected_index < list->row_count - 1) list->selected_index++;
            else list->selected_index = 0;
            return -1;
        case '\n':
        case '\r':
            return (int)list->selected_index;
        default:
            return -1;
    }
}

const char *hive_tui_list_get_selected(const hive_tui_list_t *list)
{
    if (list == NULL || list->rows == NULL || list->selected_index >= list->row_count)
        return NULL;
    return list->rows[list->selected_index];
}

void hive_tui_list_set_rows(hive_tui_list_t *list, const char **rows, size_t count)
{
    if (list == NULL) return;
    list->rows = rows;
    list->row_count = count;
    if (list->selected_index >= count) {
        list->selected_index = count > 0 ? count - 1 : 0;
    }
}

void hive_tui_list_clear_selection(hive_tui_list_t *list)
{
    if (list == NULL) return;
    list->selected_index = 0;
}
