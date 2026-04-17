#include "gtk/components/inspector_pane.h"

#include <string.h>

GtkWidget *hive_inspector_pane_new(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(scroll, "hive-inspector");
    gtk_widget_set_size_request(scroll, 240, -1);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_margin_top(vbox, 4);
    gtk_widget_set_margin_bottom(vbox, 8);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), vbox);

    g_object_set_data(G_OBJECT(scroll), "content-box", vbox);

    gtk_accessible_update_property(GTK_ACCESSIBLE(scroll),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Inspector",
                                   -1);
    return scroll;
}

GtkWidget *hive_inspector_pane_add_section(GtkWidget *pane, const char *title)
{
    GtkWidget *vbox = g_object_get_data(G_OBJECT(pane), "content-box");
    if (vbox == NULL) return NULL;

    GtkWidget *expander = gtk_expander_new(title != NULL ? title : "");
    gtk_expander_set_expanded(GTK_EXPANDER(expander), TRUE);
    gtk_widget_set_margin_start(expander, 8);
    gtk_widget_set_margin_end(expander, 8);
    gtk_widget_set_margin_bottom(expander, 4);
    gtk_box_append(GTK_BOX(vbox), expander);

    GtkWidget *section_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_set_margin_top(section_box, 4);
    gtk_expander_set_child(GTK_EXPANDER(expander), section_box);

    return section_box;
}

void hive_inspector_section_add_row(GtkWidget *section,
                                     const char *key,
                                     const char *value)
{
    if (section == NULL || key == NULL) return;

    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(row, 2);
    gtk_widget_set_margin_bottom(row, 2);
    gtk_widget_set_margin_start(row, 4);
    gtk_widget_set_margin_end(row, 4);

    GtkWidget *key_lbl = gtk_label_new(key);
    gtk_label_set_xalign(GTK_LABEL(key_lbl), 0.0);
    gtk_widget_add_css_class(key_lbl, "hive-inspector-key");
    gtk_widget_set_size_request(key_lbl, 80, -1);
    gtk_box_append(GTK_BOX(row), key_lbl);

    GtkWidget *val_lbl = gtk_label_new(value != NULL ? value : "");
    gtk_label_set_xalign(GTK_LABEL(val_lbl), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(val_lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_set_hexpand(val_lbl, TRUE);
    gtk_widget_add_css_class(val_lbl, "hive-inspector-value");
    /* Store key name for lookup in hive_inspector_section_update_row */
    g_object_set_data_full(G_OBJECT(val_lbl), "row-key",
                           g_strdup(key), g_free);
    gtk_box_append(GTK_BOX(row), val_lbl);

    gtk_box_append(GTK_BOX(section), row);
}

void hive_inspector_section_update_row(GtkWidget *section,
                                        const char *key,
                                        const char *new_value)
{
    if (section == NULL || key == NULL) return;

    GtkWidget *child = gtk_widget_get_first_child(section);
    while (child != NULL) {
        /* Each child is a GtkBox row; iterate its children to find val_lbl */
        GtkWidget *col = gtk_widget_get_first_child(child);
        while (col != NULL) {
            const char *stored = g_object_get_data(G_OBJECT(col), "row-key");
            if (stored != NULL && strcmp(stored, key) == 0) {
                gtk_label_set_text(GTK_LABEL(col),
                                   new_value != NULL ? new_value : "");
                return;
            }
            col = gtk_widget_get_next_sibling(col);
        }
        child = gtk_widget_get_next_sibling(child);
    }
}
