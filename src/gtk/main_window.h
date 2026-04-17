#ifndef HIVE_GTK_MAIN_WINDOW_H
#define HIVE_GTK_MAIN_WINDOW_H

#include <gtk/gtk.h>
#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hive_runtime hive_runtime_t;

/**
 * Create and present the main Hive application window.
 *
 * Layout:
 *   GtkApplicationWindow
 *   ├── titlebar : GtkHeaderBar  (title, search, theme toggle, Run button)
 *   └── child    : GtkBox (vertical)
 *       ├── GtkSearchBar  (hidden until search toggle pressed)
 *       ├── GtkPaned H    (outer: sidebar | right area)
 *       │   ├── sidebar   : GtkScrolledWindow > GtkListBox  (nav)
 *       │   └── GtkPaned H (inner: stack | inspector)
 *       │       ├── GtkStack  (sessions / agents / editor / settings)
 *       │       └── inspector : GtkScrolledWindow  (collapsible panels)
 *       └── status bar : GtkBox
 *
 * @param app     The GtkApplication instance.
 * @param runtime The hive runtime (may be NULL; Run button is disabled).
 * @return The new GtkApplicationWindow widget.
 */
GtkWidget *hive_main_window_new(GtkApplication *app, hive_runtime_t *runtime);

/**
 * Update the status bar message from any thread via g_idle_add.
 * Thread-safe.
 *
 * @param window  The window returned by hive_main_window_new().
 * @param message Text to display (copied internally).
 */
void hive_main_window_set_status(GtkWidget *window, const char *message);

#ifdef __cplusplus
}
#endif

#endif /* HIVE_GTK_MAIN_WINDOW_H */
