#ifndef HIVE_GTK_NAV_LIST_H
#define HIVE_GTK_NAV_LIST_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a sidebar navigation list wrapped in a GtkScrolledWindow.
 * The returned widget carries the CSS class "hive-sidebar".
 */
GtkWidget *hive_nav_list_new(void);

/**
 * Append a navigation item.
 *
 * @param nav_list  Widget returned by hive_nav_list_new().
 * @param icon_name Symbolic icon name (may be NULL).
 * @param label     Display text.
 * @param page_name Identifier stored as "page-name" GObject data on the row.
 */
void hive_nav_list_add_item(GtkWidget *nav_list,
                             const char *icon_name,
                             const char *label,
                             const char *page_name);

/** Return the inner GtkListBox (for signal connections). */
GtkWidget *hive_nav_list_get_listbox(GtkWidget *nav_list);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_GTK_NAV_LIST_H */
