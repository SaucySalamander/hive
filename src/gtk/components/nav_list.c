#include "gtk/components/nav_list.h"

GtkWidget *hive_nav_list_new(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(scroll, "hive-sidebar");
    gtk_widget_set_size_request(scroll, 200, -1);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox),
                                    GTK_SELECTION_SINGLE);
    gtk_widget_set_vexpand(listbox, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);

    g_object_set_data(G_OBJECT(scroll), "listbox", listbox);
    return scroll;
}

void hive_nav_list_add_item(GtkWidget *nav_list,
                             const char *icon_name,
                             const char *label,
                             const char *page_name)
{
    GtkWidget *listbox = g_object_get_data(G_OBJECT(nav_list), "listbox");
    if (listbox == NULL || label == NULL) return;

    GtkWidget *row_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(row_box, 4);
    gtk_widget_set_margin_bottom(row_box, 4);

    if (icon_name != NULL) {
        GtkWidget *icon = gtk_image_new_from_icon_name(icon_name);
        gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
        gtk_box_append(GTK_BOX(row_box), icon);
    }

    GtkWidget *lbl = gtk_label_new(label);
    gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
    gtk_widget_set_hexpand(lbl, TRUE);
    gtk_box_append(GTK_BOX(row_box), lbl);

    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_box);
    gtk_accessible_update_property(GTK_ACCESSIBLE(row),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL, label,
                                   -1);

    if (page_name != NULL) {
        g_object_set_data_full(G_OBJECT(row), "page-name",
                               g_strdup(page_name), g_free);
    }

    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
}

GtkWidget *hive_nav_list_get_listbox(GtkWidget *nav_list)
{
    return g_object_get_data(G_OBJECT(nav_list), "listbox");
}
