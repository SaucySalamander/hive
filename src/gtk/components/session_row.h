#ifndef HIVE_GTK_SESSION_ROW_H
#define HIVE_GTK_SESSION_ROW_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a session list-row widget.
 *
 * @param title    Primary session label (required).
 * @param subtitle Secondary metadata line (may be NULL).
 * @return A GtkBox with CSS classes "hive-session-row", intended to be
 *         wrapped in a GtkListBoxRow.
 */
GtkWidget *hive_session_row_new(const char *title, const char *subtitle);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_GTK_SESSION_ROW_H */
