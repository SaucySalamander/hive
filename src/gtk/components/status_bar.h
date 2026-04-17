#ifndef HIVE_GTK_STATUS_BAR_H
#define HIVE_GTK_STATUS_BAR_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Create a single-line status bar GtkBox with CSS class "hive-statusbar".
 */
GtkWidget *hive_status_bar_new(void);

/** Set the message shown in the status bar. */
void hive_status_bar_set_message(GtkWidget *bar, const char *message);

/** Convenience: return the inner GtkLabel. */
GtkWidget *hive_status_bar_get_label(GtkWidget *bar);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_GTK_STATUS_BAR_H */
