#include "gtk/main_window.h"
#include "gtk/components/nav_list.h"
#include "gtk/components/session_row.h"
#include "gtk/components/inspector_pane.h"
#include "gtk/components/status_bar.h"
#include "core/runtime.h"
#include "logging/logger.h"

#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <pthread.h>
#include <stdio.h>
#include <math.h>

/* ================================================================
 * Hex button — agents/bees page
 * ================================================================ */

typedef struct {
    char *label;
    gboolean pressed;
    GtkWidget *area;
} hex_btn_t;

static void hex_btn_free(gpointer data)
{
    hex_btn_t *h = data;
    if (h == NULL) return;
    g_free(h->label);
    g_free(h);
}

static gboolean point_in_hexagon(double x, double y,
                                  double *pts, int n)
{
    int inside = 0;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        double xi = pts[2 * i], yi = pts[2 * i + 1];
        double xj = pts[2 * j], yj = pts[2 * j + 1];
        if (((yi > y) != (yj > y)) &&
            (x < (xj - xi) * (y - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

static void hex_draw_fn(GtkDrawingArea *area, cairo_t *cr,
                         int width, int height, gpointer user_data)
{
    hex_btn_t *h = user_data;
    if (h == NULL) return;

    double cx = width  / 2.0;
    double cy = height / 2.0;
    double r  = (width < height ? width : height) * 0.42;
    double pts[12];
    double a0 = M_PI / 6.0; /* flat-top */
    for (int i = 0; i < 6; ++i) {
        double a = a0 + i * 2.0 * M_PI / 6.0;
        pts[2 * i]     = cx + r * cos(a);
        pts[2 * i + 1] = cy + r * sin(a);
    }

    cairo_save(cr);
    if (h->pressed)
        cairo_set_source_rgb(cr, 0.96, 0.65, 0.14); /* amber active */
    else
        cairo_set_source_rgb(cr, 0.95, 0.95, 0.95);
    cairo_move_to(cr, pts[0], pts[1]);
    for (int i = 1; i < 6; ++i) cairo_line_to(cr, pts[2*i], pts[2*i+1]);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.78, 0.64, 0.0);
    cairo_set_line_width(cr, 2.0);
    cairo_stroke(cr);

    PangoLayout *layout =
        gtk_widget_create_pango_layout(GTK_WIDGET(area), h->label);
    int lw, lh;
    pango_layout_get_pixel_size(layout, &lw, &lh);
    cairo_move_to(cr, cx - lw / 2.0, cy - lh / 2.0);
    cairo_set_source_rgb(cr, 0.11, 0.11, 0.11);
    pango_cairo_show_layout(cr, layout);
    g_object_unref(layout);
    cairo_restore(cr);
}

static void hex_gesture_pressed(GtkGestureClick *gesture,
                                  gint n_press,
                                  gdouble x, gdouble y,
                                  gpointer user_data)
{
    (void)gesture;
    (void)n_press;
    hex_btn_t *h = user_data;
    if (h == NULL || h->area == NULL) return;
    int w = gtk_widget_get_width(h->area);
    int ht = gtk_widget_get_height(h->area);
    double cx = w / 2.0, cy = ht / 2.0;
    double r  = (w < ht ? w : ht) * 0.42;
    double pts[12];
    double a0 = M_PI / 6.0;
    for (int i = 0; i < 6; ++i) {
        double a = a0 + i * 2.0 * M_PI / 6.0;
        pts[2*i]   = cx + r * cos(a);
        pts[2*i+1] = cy + r * sin(a);
    }
    if (point_in_hexagon(x, y, pts, 6)) {
        h->pressed = !h->pressed;
        gtk_widget_queue_draw(h->area);
    }
}

static GtkWidget *create_hex_button(const char *label, int size)
{
    hex_btn_t *h = g_new0(hex_btn_t, 1);
    h->label   = g_strdup(label != NULL ? label : "");
    h->pressed = FALSE;

    GtkWidget *area = gtk_drawing_area_new();
    h->area = area;
    gtk_widget_set_size_request(area, size, size);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area),
                                   hex_draw_fn, h, hex_btn_free);

    GtkGesture *g = gtk_gesture_click_new();
    g_signal_connect(g, "pressed", G_CALLBACK(hex_gesture_pressed), h);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(g));

    gtk_widget_set_tooltip_text(area, label != NULL ? label : "Agent");
    gtk_accessible_update_property(GTK_ACCESSIBLE(area),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   label != NULL ? label : "Agent",
                                   -1);
    return area;
}

/* ================================================================
 * Page builders
 * ================================================================ */

static GtkWidget *build_agents_page(void)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_halign(page, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(page, GTK_ALIGN_CENTER);
    gtk_widget_set_vexpand(page, TRUE);
    gtk_widget_set_hexpand(page, TRUE);

    GtkWidget *title = gtk_label_new("Agents");
    gtk_widget_add_css_class(title, "hive-session-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.5);
    gtk_widget_set_margin_bottom(title, 4);
    gtk_box_append(GTK_BOX(page), title);

    GtkWidget *grid = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    int hex_size = 120;
    int id = 1;
    for (int row = 0; row < 4; ++row) {
        GtkWidget *hrow = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
        if (row % 2 == 1)
            gtk_widget_set_margin_start(hrow, hex_size / 2);
        int cols = (row % 2 == 0) ? 4 : 3;
        for (int col = 0; col < cols; ++col) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Agent %d", id++);
            gtk_box_append(GTK_BOX(hrow), create_hex_button(buf, hex_size));
        }
        gtk_box_append(GTK_BOX(grid), hrow);
    }
    gtk_box_append(GTK_BOX(page), grid);
    return page;
}

static GtkWidget *build_sessions_page(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *listbox = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(listbox),
                                    GTK_SELECTION_SINGLE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), listbox);

    /* Placeholder rows */
    const char *titles[]    = { "Refactor auth module",
                                 "Write unit tests for parser",
                                 "Scaffold REST endpoints" };
    const char *subtitles[] = { "3 iterations · 2 h ago",
                                 "1 iteration · Yesterday",
                                 "5 iterations · 3 days ago" };
    for (int i = 0; i < 3; ++i) {
        GtkWidget *row_content =
            hive_session_row_new(titles[i], subtitles[i]);
        GtkWidget *row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_content);
        gtk_list_box_append(GTK_LIST_BOX(listbox), row);
    }
    return scroll;
}

static GtkWidget *build_editor_page(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *text = gtk_text_view_new();
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text), GTK_WRAP_WORD_CHAR);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(text), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(text), 12);
    gtk_text_view_set_right_margin(GTK_TEXT_VIEW(text), 12);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(text), 8);
    gtk_text_buffer_set_text(
        gtk_text_view_get_buffer(GTK_TEXT_VIEW(text)),
        "/* Hive editor — connect to session output here */\n", -1);
    gtk_widget_set_hexpand(text, TRUE);
    gtk_widget_set_vexpand(text, TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), text);
    gtk_widget_set_tooltip_text(text, "Editor — session output");
    return scroll;
}

static GtkWidget *build_settings_page(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 12);
    gtk_widget_set_margin_top(grid, 16);
    gtk_widget_set_margin_start(grid, 16);
    gtk_widget_set_margin_end(grid, 16);
    gtk_widget_set_hexpand(grid, TRUE);

    const char *field_labels[] = {
        "Workspace root", "User prompt", "Max iterations", NULL
    };
    for (int i = 0; field_labels[i] != NULL; ++i) {
        GtkWidget *lbl = gtk_label_new(field_labels[i]);
        gtk_label_set_xalign(GTK_LABEL(lbl), 1.0);
        gtk_grid_attach(GTK_GRID(grid), lbl, 0, i, 1, 1);

        GtkWidget *entry = gtk_entry_new();
        gtk_widget_set_hexpand(entry, TRUE);
        gtk_accessible_update_property(GTK_ACCESSIBLE(entry),
                                       GTK_ACCESSIBLE_PROPERTY_LABEL,
                                       field_labels[i], -1);
        gtk_grid_attach(GTK_GRID(grid), entry, 1, i, 1, 1);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), grid);
    return scroll;
}

/* ================================================================
 * Runtime threading (Prompt 06 integration)
 * ================================================================ */

typedef struct {
    hive_runtime_t *runtime;
    GtkWidget      *status_bar;
    GtkWidget      *run_button;
} mw_run_ctx_t;

typedef struct {
    GtkWidget *status_bar;
    GtkWidget *run_button;
    char      *msg;
} mw_finish_data_t;

static gboolean on_runtime_done_idle(gpointer user_data)
{
    mw_finish_data_t *f = user_data;
    hive_status_bar_set_message(f->status_bar, f->msg);
    gtk_widget_set_sensitive(f->run_button, TRUE);
    g_free(f->msg);
    g_free(f);
    return G_SOURCE_REMOVE;
}

static void *runtime_thread_fn(void *arg)
{
    mw_run_ctx_t *ctx = arg;

    if (ctx->runtime != NULL &&
        ctx->runtime->logger.initialized) {
        hive_logger_log(&ctx->runtime->logger,
                        HIVE_LOG_INFO, "gtk", "main_window",
                        "runtime thread started");
    }

    hive_status_t st = hive_runtime_run(ctx->runtime);

    const char *text = (st == HIVE_STATUS_OK)
                       ? "Runtime finished: OK"
                       : "Runtime finished: ERROR";
    mw_finish_data_t *f = g_new0(mw_finish_data_t, 1);
    f->status_bar = ctx->status_bar;
    f->run_button = ctx->run_button;
    f->msg        = g_strdup(text);
    g_idle_add(on_runtime_done_idle, f);
    return NULL;
}

static void on_run_clicked(GtkButton *btn, gpointer user_data)
{
    mw_run_ctx_t *ctx = user_data;
    if (ctx == NULL || ctx->runtime == NULL) return;
    gtk_widget_set_sensitive(GTK_WIDGET(btn), FALSE);
    hive_status_bar_set_message(ctx->status_bar, "Runtime running…");
    pthread_t tid;
    pthread_create(&tid, NULL, runtime_thread_fn, ctx);
    pthread_detach(tid);
}

/* ================================================================
 * Nav → Stack wiring
 * ================================================================ */

static void on_nav_row_activated(GtkListBox *listbox,
                                  GtkListBoxRow *row,
                                  gpointer user_data)
{
    (void)listbox;
    GtkStack *stack = GTK_STACK(user_data);
    const char *page = g_object_get_data(G_OBJECT(row), "page-name");
    if (page != NULL)
        gtk_stack_set_visible_child_name(stack, page);
}

/* ================================================================
 * Theme toggle (dark / light)
 * ================================================================ */

static void on_theme_toggle(GtkButton *btn, gpointer user_data)
{
    GtkWidget *window = GTK_WIDGET(user_data);
    if (gtk_widget_has_css_class(window, "hive-dark")) {
        gtk_widget_remove_css_class(window, "hive-dark");
        gtk_widget_set_tooltip_text(GTK_WIDGET(btn),
                                    "Switch to dark mode");
    } else {
        gtk_widget_add_css_class(window, "hive-dark");
        gtk_widget_set_tooltip_text(GTK_WIDGET(btn),
                                    "Switch to light mode");
    }
}

/* ================================================================
 * Search bar toggle
 * ================================================================ */

static void on_search_toggle(GtkToggleButton *btn, gpointer user_data)
{
    GtkSearchBar *bar = GTK_SEARCH_BAR(user_data);
    gtk_search_bar_set_search_mode(bar,
                                   gtk_toggle_button_get_active(btn));
}

/* ================================================================
 * hive_main_window_set_status helper
 * ================================================================ */

typedef struct { GtkWidget *bar; char *msg; } set_status_idle_t;

static gboolean set_status_idle(gpointer data)
{
    set_status_idle_t *s = data;
    hive_status_bar_set_message(s->bar, s->msg);
    g_free(s->msg);
    g_free(s);
    return G_SOURCE_REMOVE;
}

void hive_main_window_set_status(GtkWidget *window, const char *message)
{
    GtkWidget *bar = g_object_get_data(G_OBJECT(window), "status-bar");
    if (bar == NULL) return;
    set_status_idle_t *s = g_new0(set_status_idle_t, 1);
    s->bar = bar;
    s->msg = g_strdup(message != NULL ? message : "");
    g_idle_add(set_status_idle, s);
}

/* ================================================================
 * Main window constructor
 * ================================================================ */

GtkWidget *hive_main_window_new(GtkApplication *app,
                                 hive_runtime_t  *runtime)
{
    GtkWidget *window =
        gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Hive");
    gtk_window_set_default_size(GTK_WINDOW(window), 1200, 800);
    gtk_widget_add_css_class(window, "hive-window");

    /* ---- Header bar ---- */
    GtkWidget *header = gtk_header_bar_new();
    gtk_header_bar_set_show_title_buttons(
        GTK_HEADER_BAR(header), TRUE);

    /* Title widget */
    GtkWidget *title_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget *title_lbl = gtk_label_new("Hive");
    gtk_widget_add_css_class(title_lbl, "title");
    gtk_box_append(GTK_BOX(title_box), title_lbl);
    gtk_header_bar_set_title_widget(GTK_HEADER_BAR(header), title_box);

    /* Search toggle (start) */
    GtkWidget *search_btn = gtk_toggle_button_new();
    GtkWidget *search_icon =
        gtk_image_new_from_icon_name("system-search-symbolic");
    gtk_button_set_child(GTK_BUTTON(search_btn), search_icon);
    gtk_widget_set_tooltip_text(search_btn, "Search (Ctrl+F)");
    gtk_accessible_update_property(GTK_ACCESSIBLE(search_btn),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Toggle search bar", -1);
    gtk_header_bar_pack_start(GTK_HEADER_BAR(header), search_btn);

    /* Theme toggle (end) */
    GtkWidget *theme_btn =
        gtk_button_new_from_icon_name("weather-clear-night-symbolic");
    gtk_widget_set_tooltip_text(theme_btn, "Toggle dark mode");
    gtk_accessible_update_property(GTK_ACCESSIBLE(theme_btn),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Toggle dark mode", -1);
    g_signal_connect(theme_btn, "clicked",
                     G_CALLBACK(on_theme_toggle), window);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), theme_btn);

    /* Run button (end) */
    GtkWidget *run_btn = gtk_button_new_with_label("Run");
    gtk_widget_add_css_class(run_btn, "suggested-action");
    gtk_widget_set_tooltip_text(run_btn, "Start runtime (Ctrl+R)");
    gtk_accessible_update_property(GTK_ACCESSIBLE(run_btn),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Start runtime", -1);
    gtk_header_bar_pack_end(GTK_HEADER_BAR(header), run_btn);

    gtk_window_set_titlebar(GTK_WINDOW(window), header);

    /* ---- Content stack ---- */
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack),
                                  GTK_STACK_TRANSITION_TYPE_CROSSFADE);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 150);
    gtk_widget_set_hexpand(stack, TRUE);
    gtk_widget_set_vexpand(stack, TRUE);

    gtk_stack_add_named(GTK_STACK(stack),
                        build_sessions_page(), "sessions");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_agents_page(),   "agents");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_editor_page(),   "editor");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_settings_page(), "settings");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "agents");

    /* ---- Sidebar nav ---- */
    GtkWidget *nav = hive_nav_list_new();
    hive_nav_list_add_item(nav, "view-list-symbolic",
                            "Sessions",  "sessions");
    hive_nav_list_add_item(nav, "applications-science-symbolic",
                            "Agents",    "agents");
    hive_nav_list_add_item(nav, "accessories-text-editor-symbolic",
                            "Editor",    "editor");
    hive_nav_list_add_item(nav, "preferences-system-symbolic",
                            "Settings",  "settings");

    GtkWidget *listbox = hive_nav_list_get_listbox(nav);
    g_signal_connect(listbox, "row-activated",
                     G_CALLBACK(on_nav_row_activated), stack);
    /* Select "Agents" row (index 1) by default */
    GtkListBoxRow *default_row =
        gtk_list_box_get_row_at_index(GTK_LIST_BOX(listbox), 1);
    if (default_row != NULL)
        gtk_list_box_select_row(GTK_LIST_BOX(listbox), default_row);

    /* ---- Inspector ---- */
    GtkWidget *inspector = hive_inspector_pane_new();
    GtkWidget *session_section =
        hive_inspector_pane_add_section(inspector, "Session");
    hive_inspector_section_add_row(session_section, "Status",    "Idle");
    hive_inspector_section_add_row(session_section, "Workspace", "—");
    hive_inspector_section_add_row(session_section, "Iteration", "0");

    GtkWidget *runtime_section =
        hive_inspector_pane_add_section(inspector, "Runtime");
    hive_inspector_section_add_row(runtime_section, "Mode",
                                    runtime != NULL ? "live" : "no runtime");
    hive_inspector_section_add_row(runtime_section, "Mock LLM",
                                    runtime != NULL &&
                                    runtime->options.use_mock_inference
                                    ? "yes" : "no");

    /* ---- Status bar ---- */
    GtkWidget *statusbar = hive_status_bar_new();
    g_object_set_data(G_OBJECT(window), "status-bar", statusbar);

    /* ---- Inner paned: stack | inspector ---- */
    GtkWidget *inner = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(inner), stack);
    gtk_paned_set_end_child(GTK_PANED(inner), inspector);
    gtk_paned_set_resize_start_child(GTK_PANED(inner), TRUE);
    gtk_paned_set_resize_end_child(GTK_PANED(inner), FALSE);
    gtk_paned_set_shrink_start_child(GTK_PANED(inner), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(inner), FALSE);
    gtk_widget_set_hexpand(inner, TRUE);
    gtk_widget_set_vexpand(inner, TRUE);

    /* ---- Outer paned: nav | inner ---- */
    GtkWidget *outer = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_start_child(GTK_PANED(outer), nav);
    gtk_paned_set_end_child(GTK_PANED(outer), inner);
    gtk_paned_set_resize_start_child(GTK_PANED(outer), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(outer), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(outer), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(outer), FALSE);
    gtk_widget_set_hexpand(outer, TRUE);
    gtk_widget_set_vexpand(outer, TRUE);

    /* ---- Search bar ---- */
    GtkWidget *search_entry = gtk_search_entry_new();
    gtk_widget_set_hexpand(search_entry, TRUE);
    gtk_accessible_update_property(GTK_ACCESSIBLE(search_entry),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   "Search", -1);

    GtkWidget *search_bar = gtk_search_bar_new();
    gtk_search_bar_set_child(GTK_SEARCH_BAR(search_bar), search_entry);
    gtk_search_bar_set_show_close_button(GTK_SEARCH_BAR(search_bar), TRUE);
    gtk_search_bar_connect_entry(GTK_SEARCH_BAR(search_bar),
                                  GTK_EDITABLE(search_entry));

    g_signal_connect(search_btn, "toggled",
                     G_CALLBACK(on_search_toggle), search_bar);

    /* ---- Root content box ---- */
    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(content), search_bar);
    gtk_box_append(GTK_BOX(content), outer);
    gtk_box_append(GTK_BOX(content), statusbar);
    gtk_window_set_child(GTK_WINDOW(window), content);

    /* ---- Run button wiring ---- */
    mw_run_ctx_t *run_ctx = g_new0(mw_run_ctx_t, 1);
    run_ctx->runtime    = runtime;
    run_ctx->status_bar = statusbar;
    run_ctx->run_button = run_btn;
    /* free run_ctx when the window is destroyed */
    g_object_set_data_full(G_OBJECT(window), "run-ctx", run_ctx, g_free);

    if (runtime == NULL) {
        gtk_widget_set_sensitive(run_btn, FALSE);
    } else {
        g_signal_connect(run_btn, "clicked",
                         G_CALLBACK(on_run_clicked), run_ctx);
    }

    /* ---- Keyboard shortcuts (Prompt 05) ---- */
    GtkShortcutController *sc =
        GTK_SHORTCUT_CONTROLLER(gtk_shortcut_controller_new());
    gtk_shortcut_controller_set_scope(sc, GTK_SHORTCUT_SCOPE_MANAGED);

    /* Ctrl+F — focus search */
    gtk_shortcut_controller_add_shortcut(sc,
        gtk_shortcut_new(
            gtk_keyval_trigger_new(GDK_KEY_f, GDK_CONTROL_MASK),
            gtk_callback_action_new(
                (GtkShortcutFunc)(void (*)(void))gtk_toggle_button_set_active,
                search_btn, NULL)));

    gtk_widget_add_controller(window, GTK_EVENT_CONTROLLER(sc));

    gtk_window_present(GTK_WINDOW(window));
    return window;
}
