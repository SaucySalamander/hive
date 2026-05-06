#include "tui/components/menu.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif
#endif

#include <string.h>

void hive_tui_menu_init(hive_tui_menu_t *menu, const char **items, size_t count,
                        int start_row, int start_col, int height)
{
    if (menu == NULL) return;
    menu->items = items;
    menu->item_count = count;
    menu->selected_index = 0;
    menu->start_row = start_row;
    menu->start_col = start_col;
    menu->height = height;
}

void hive_tui_menu_draw(hive_tui_menu_t *menu)
{
    if (menu == NULL || menu->items == NULL) return;

#if HIVE_HAVE_NCURSES
    int row = menu->start_row;
    int visible = menu->height > 0 ? menu->height : (int)menu->item_count;
    int start_item = menu->selected_index > (size_t)visible ?
                     (int)menu->selected_index - visible + 1 : 0;

    for (int i = 0; i < visible && start_item + i < (int)menu->item_count; i++) {
        const char *item = menu->items[start_item + i];
        int attr = (start_item + i == (int)menu->selected_index) ?
                   A_REVERSE : A_NORMAL;
        
        attron(attr);
        mvprintw(row + i, menu->start_col, "%-60s", item ? item : "");
        attroff(attr);
    }
    refresh();
#endif
}

int hive_tui_menu_handle_key(hive_tui_menu_t *menu, int key)
{
    if (menu == NULL) return -1;

    switch (key) {
        case KEY_UP:
            if (menu->selected_index > 0) menu->selected_index--;
            else if (menu->item_count > 0) menu->selected_index = menu->item_count - 1;
            return -1;
        case KEY_DOWN:
            if (menu->selected_index < menu->item_count - 1) menu->selected_index++;
            else menu->selected_index = 0;
            return -1;
        case '\n':
        case '\r':
            return (int)menu->selected_index;
        default:
            return -1;
    }
}

const char *hive_tui_menu_get_selected(const hive_tui_menu_t *menu)
{
    if (menu == NULL || menu->items == NULL || menu->selected_index >= menu->item_count)
        return NULL;
    return menu->items[menu->selected_index];
}
