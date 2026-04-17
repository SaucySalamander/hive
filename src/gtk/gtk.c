#include <gtk/gtk.h>
#include "gtk/hive_gtk.h"

#include "core/runtime.h"
#include "logging/logger.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pango/pangocairo.h>

typedef struct {
    hive_runtime_t *runtime;
    GtkWidget *status_label;
    GtkWidget *start_button;
} gtk_run_data_t;

typedef struct {
    GtkWidget *label;
    char *msg;
    gtk_run_data_t *run_data;
} finish_data_t;

static gboolean on_runtime_finished_cb(gpointer user_data)
{
    finish_data_t *f = user_data;
    if (f == NULL) return G_SOURCE_REMOVE;
    gtk_label_set_text(GTK_LABEL(f->label), f->msg);
    if (f->run_data != NULL && f->run_data->start_button != NULL) {
        gtk_widget_set_sensitive(f->run_data->start_button, TRUE);
    }
    g_free(f->msg);
    g_free(f);
    return G_SOURCE_REMOVE;
}

void *runtime_thread_fn(void *arg)
{
    gtk_run_data_t *d = arg;
    if (d == NULL) return NULL;

    if (d->runtime != NULL && d->runtime->logger.initialized) {
        hive_logger_log(&d->runtime->logger, HIVE_LOG_INFO, "gtk", "runtime", "starting runtime thread");
    }

    hive_status_t status = hive_runtime_run(d->runtime);

    const char *text = (status == HIVE_STATUS_OK) ? "Runtime finished: OK" : "Runtime finished: ERROR";
    finish_data_t *f = g_new0(finish_data_t, 1);
    f->label = d->status_label;
    f->msg = g_strdup(text);
    f->run_data = d;
    g_idle_add(on_runtime_finished_cb, f);

    return NULL;
}

static void nav_button_clicked(GtkButton *button, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    const gchar *page = g_object_get_data(G_OBJECT(button), "page-name");
    if (page != NULL) {
        gtk_stack_set_visible_child_name(stack, page);
    }
}

typedef struct {
    char *label;
    gboolean pressed;
    GtkWidget *area;
} hex_button_t;

static void hex_button_free(gpointer data)
{
    hex_button_t *hb = data;
    if (hb == NULL) return;
    g_free(hb->label);
    g_free(hb);
}

static gboolean point_in_polygon(double x, double y, double *pts, int npoints)
{
    int i, j, c = 0;
    for (i = 0, j = npoints - 1; i < npoints; j = i++) {
        double xi = pts[2*i], yi = pts[2*i+1];
        double xj = pts[2*j], yj = pts[2*j+1];
        if ( ((yi > y) != (yj > y)) && (x < (xj - xi) * (y - yi) / (yj - yi) + xi) )
            c = !c;
    }
    return c;
}

static void hex_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
    hex_button_t *hb = user_data;
    if (hb == NULL) return;

    double cx = width / 2.0;
    double cy = height / 2.0;
    double r = (width < height ? width : height) * 0.42;
    double pts[12];
    double angle0 = M_PI / 6.0; /* flat-top hexagon */
    for (int i = 0; i < 6; ++i) {
        double a = angle0 + i * 2.0 * M_PI / 6.0;
        pts[2*i] = cx + r * cos(a);
        pts[2*i+1] = cy + r * sin(a);
    }

    cairo_save(cr);
    if (hb->pressed) {
        cairo_set_source_rgb(cr, 0.95, 0.85, 0.2);
    } else {
        cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    }
    cairo_move_to(cr, pts[0], pts[1]);
    for (int i = 1; i < 6; ++i) cairo_line_to(cr, pts[2*i], pts[2*i+1]);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.2, 0.2, 0.2);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    PangoLayout *layout = gtk_widget_create_pango_layout(GTK_WIDGET(area), hb->label);
    int lw, lh;
    pango_layout_get_pixel_size(layout, &lw, &lh);
    cairo_move_to(cr, cx - lw / 2.0, cy - lh / 2.0);
    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);

    cairo_restore(cr);
}

static void hex_pressed(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data)
{
    (void)gesture;
    (void)n_press;
    hex_button_t *hb = user_data;
    if (hb == NULL || hb->area == NULL) return;
    GtkWidget *area = hb->area;
    int width = gtk_widget_get_width(area);
    int height = gtk_widget_get_height(area);
    double cx = width / 2.0;
    double cy = height / 2.0;
    double r = (width < height ? width : height) * 0.42;
    double pts[12];
    double angle0 = M_PI / 6.0;
    for (int i = 0; i < 6; ++i) {
        double a = angle0 + i * 2.0 * M_PI / 6.0;
        pts[2*i] = cx + r * cos(a);
        pts[2*i+1] = cy + r * sin(a);
    }

    if (point_in_polygon(x, y, pts, 6)) {
        hb->pressed = !hb->pressed;
        gtk_widget_queue_draw(area);
        g_print("Hex clicked: %s\n", hb->label);
    }
}

static GtkWidget *create_hex_button(const char *label, int size)
{
    hex_button_t *hb = g_new0(hex_button_t, 1);
    hb->label = g_strdup(label ? label : "");
    hb->pressed = FALSE;

    GtkWidget *area = gtk_drawing_area_new();
    hb->area = area;
    gtk_widget_set_size_request(area, size, size);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area), hex_draw, hb, hex_button_free);

    GtkGesture *g = gtk_gesture_click_new();
    g_signal_connect(g, "pressed", G_CALLBACK(hex_pressed), hb);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(g));

    return area;
}

static void on_activate(GApplication *app, gpointer user_data)
{
    hive_runtime_t *runtime = user_data;
    (void)runtime;
    GtkWidget *window = gtk_application_window_new(GTK_APPLICATION(app));
    gtk_window_set_title(GTK_WINDOW(window), "hive - GTK4");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 700);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_window_set_child(GTK_WINDOW(window), vbox);

    GtkWidget *header = gtk_label_new("Hive — an agent harness");
    gtk_label_set_xalign(GTK_LABEL(header), 0.5);
    gtk_widget_set_halign(header, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(vbox), header);

    GtkWidget *status = gtk_label_new("Status: Ready");
    gtk_box_append(GTK_BOX(vbox), status);

    /* Main stacked pages */
    GtkWidget *stack = gtk_stack_new();
    gtk_widget_set_vexpand(stack, TRUE);

    /* Bees page (hexagon placeholders) */
    GtkWidget *bees_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(bees_page, GTK_ALIGN_CENTER);
    GtkWidget *bees_title = gtk_label_new("Bees (Agents)");
    gtk_label_set_xalign(GTK_LABEL(bees_title), 0.5);
    gtk_box_append(GTK_BOX(bees_page), bees_title);

    /* Honeycomb-like staggered rows (tighter spacing) */
    GtkWidget *hex_rows = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    int hex_size = 120;
    int id = 1;
    int rows = 4;
    for (int r = 0; r < rows; ++r) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        if (r % 2 == 1) gtk_widget_set_margin_start(row, hex_size / 2);
        int cols = (r % 2 == 0) ? 4 : 3;
        for (int c = 0; c < cols; ++c) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Agent %d", id++);
            GtkWidget *a = create_hex_button(buf, hex_size);
            gtk_box_append(GTK_BOX(row), a);
        }
        gtk_box_append(GTK_BOX(hex_rows), row);
    }
    gtk_box_append(GTK_BOX(bees_page), hex_rows);

    /* Queen page */
    GtkWidget *queen_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *queen_title = gtk_label_new("Queen — Orchestration");
    gtk_box_append(GTK_BOX(queen_page), queen_title);
    GtkWidget *queen_status = gtk_label_new("No active work");
    gtk_box_append(GTK_BOX(queen_page), queen_status);

    /* Flowers page */
    GtkWidget *flowers_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    GtkWidget *flowers_title = gtk_label_new("Flowers — Knowledge");
    gtk_box_append(GTK_BOX(flowers_page), flowers_title);
    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
                             "LLM Wiki / Knowledge Graph\n\n(Edit this area when connected)", -1);
    gtk_box_append(GTK_BOX(flowers_page), text);

    gtk_stack_add_named(GTK_STACK(stack), bees_page, "bees");
    gtk_stack_add_named(GTK_STACK(stack), queen_page, "queen");
    gtk_stack_add_named(GTK_STACK(stack), flowers_page, "flowers");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "bees");

    gtk_box_append(GTK_BOX(vbox), stack);

    /* Bottom navigation */
    GtkWidget *bottom = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(bottom, GTK_ALIGN_CENTER);

    GtkWidget *bees_btn = gtk_button_new_with_label("Bees");
    g_object_set_data(G_OBJECT(bees_btn), "page-name", "bees");
    g_signal_connect(bees_btn, "clicked", G_CALLBACK(nav_button_clicked), stack);
    gtk_box_append(GTK_BOX(bottom), bees_btn);

    GtkWidget *queen_btn = gtk_button_new_with_label("Queen");
    g_object_set_data(G_OBJECT(queen_btn), "page-name", "queen");
    g_signal_connect(queen_btn, "clicked", G_CALLBACK(nav_button_clicked), stack);
    gtk_box_append(GTK_BOX(bottom), queen_btn);

    GtkWidget *flowers_btn = gtk_button_new_with_label("Flowers");
    g_object_set_data(G_OBJECT(flowers_btn), "page-name", "flowers");
    g_signal_connect(flowers_btn, "clicked", G_CALLBACK(nav_button_clicked), stack);
    gtk_box_append(GTK_BOX(bottom), flowers_btn);

    gtk_box_append(GTK_BOX(vbox), bottom);

    gtk_window_present(GTK_WINDOW(window));
}

hive_status_t hive_gtk_run(hive_runtime_t *runtime)
{
    if (runtime == NULL) return HIVE_STATUS_INVALID_ARGUMENT;

    /* Create a GTK application and show a small control window that can
       start the runtime in a background thread. */
    GtkApplication *app = gtk_application_new("org.hive.hive", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), runtime);
    int status = g_application_run(G_APPLICATION(app), 0, NULL);
    g_object_unref(app);

    return (status == 0) ? HIVE_STATUS_OK : HIVE_STATUS_ERROR;
}
