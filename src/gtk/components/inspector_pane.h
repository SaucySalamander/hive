#ifndef HIVE_GTK_INSPECTOR_PANE_H
#define HIVE_GTK_INSPECTOR_PANE_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create the inspector/property pane wrapped in a GtkScrolledWindow.
 * CSS class "hive-inspector".
 */
GtkWidget *hive_inspector_pane_new(void);

/**
 * Add a collapsible section to the inspector.
 *
 * @param pane  Widget returned by hive_inspector_pane_new().
 * @param title Section heading text.
 * @return The inner GtkBox to which property rows should be appended.
 */
GtkWidget *hive_inspector_pane_add_section(GtkWidget *pane,
                                            const char *title);

/**
 * Append a key/value property row to a section box returned by
 * hive_inspector_pane_add_section().
 */
void hive_inspector_section_add_row(GtkWidget *section,
                                     const char *key,
                                     const char *value);

/**
 * Update the value label of an existing property row.
 * Locates the row by iterating children; prefer caching the label widget
 * directly in performance-sensitive code.
 */
void hive_inspector_section_update_row(GtkWidget *section,
                                        const char *key,
                                        const char *new_value);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_GTK_INSPECTOR_PANE_H */
