#include <gtk/gtk.h>
#include "gtk/hive_gtk.h"
#include "gtk/main_window.h"
#include "core/runtime.h"

/* ----------------------------------------------------------------
 * CSS theme loader
 *
 * hive-theme.css is embedded as a GResource (compiled from
 * src/gtk/hive.gresource.xml by glib-compile-resources).  The
 * generated object registers the resource via a constructor;
 * we only need to install the CSS provider here.
 * ---------------------------------------------------------------- */
static void load_theme_css(void)
{
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_resource(
        provider, "/org/hive/hive/hive-theme.css");
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

/* ----------------------------------------------------------------
 * GApplication "activate" callback
 * ---------------------------------------------------------------- */
static void on_activate(GApplication *app, gpointer user_data)
{
    load_theme_css();
    hive_main_window_new(GTK_APPLICATION(app),
                         (hive_runtime_t *)user_data);
}

/* ----------------------------------------------------------------
 * Public entry point
 * ---------------------------------------------------------------- */
hive_status_t hive_gtk_run(hive_runtime_t *runtime)
{
    if (runtime == NULL) return HIVE_STATUS_INVALID_ARGUMENT;

    GtkApplication *app =
        gtk_application_new("org.hive.hive",
                             G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), runtime);
    int rc = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return (rc == 0) ? HIVE_STATUS_OK : HIVE_STATUS_ERROR;
}
