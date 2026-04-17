#include "gtk/components/session_row.h"

GtkWidget *hive_session_row_new(const char *title, const char *subtitle)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_widget_add_css_class(box, "hive-session-row");
    gtk_widget_set_margin_top(box, 6);
    gtk_widget_set_margin_bottom(box, 6);
    gtk_widget_set_margin_start(box, 10);
    gtk_widget_set_margin_end(box, 10);

    GtkWidget *title_lbl = gtk_label_new(title != NULL ? title : "Untitled");
    gtk_label_set_xalign(GTK_LABEL(title_lbl), 0.0);
    gtk_label_set_ellipsize(GTK_LABEL(title_lbl), PANGO_ELLIPSIZE_END);
    gtk_widget_add_css_class(title_lbl, "hive-session-title");
    gtk_box_append(GTK_BOX(box), title_lbl);

    if (subtitle != NULL && subtitle[0] != '\0') {
        GtkWidget *sub_lbl = gtk_label_new(subtitle);
        gtk_label_set_xalign(GTK_LABEL(sub_lbl), 0.0);
        gtk_label_set_ellipsize(GTK_LABEL(sub_lbl), PANGO_ELLIPSIZE_END);
        gtk_widget_add_css_class(sub_lbl, "hive-session-subtitle");
        gtk_box_append(GTK_BOX(box), sub_lbl);
    }

    gtk_accessible_update_property(GTK_ACCESSIBLE(box),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   title != NULL ? title : "Session",
                                   -1);
    return box;
}
