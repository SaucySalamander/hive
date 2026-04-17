#include "gtk/components/status_bar.h"

GtkWidget *hive_status_bar_new(void)
{
    GtkWidget *bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(bar, "hive-statusbar");

    GtkWidget *label = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0);
    gtk_widget_set_hexpand(label, TRUE);
    gtk_box_append(GTK_BOX(bar), label);

    g_object_set_data(G_OBJECT(bar), "label", label);

    gtk_accessible_update_property(GTK_ACCESSIBLE(bar),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Status bar",
                                   -1);
    return bar;
}

void hive_status_bar_set_message(GtkWidget *bar, const char *message)
{
    GtkWidget *label = g_object_get_data(G_OBJECT(bar), "label");
    if (label != NULL) {
        gtk_label_set_text(GTK_LABEL(label),
                           message != NULL ? message : "");
    }
}

GtkWidget *hive_status_bar_get_label(GtkWidget *bar)
{
    return g_object_get_data(G_OBJECT(bar), "label");
}
