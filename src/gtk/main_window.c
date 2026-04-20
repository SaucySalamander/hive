#include "gtk/main_window.h"
#include "gtk/components/nav_list.h"
#include "gtk/components/session_row.h"
#include "gtk/components/inspector_pane.h"
#include "gtk/components/status_bar.h"
#include "core/dynamics/dynamics.h"
#include "core/queen/queen.h"
#include "core/runtime.h"
#include "common/logging/logger.h"

#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ================================================================
 * Shared dynamics state — initialised once, ticked by a timer
 * ================================================================ */
static hive_dynamics_t g_dynamics;
static gboolean g_dynamics_inited = FALSE;

#if 0
/* ================================================================
 * Hex button — agents/bees page
 * ================================================================ */

typedef struct {
    char *label;
    gboolean pressed;
    // gtk_box_append(GTK_BOX(page), build_stats_strip());
} hex_btn_t;

static void hex_btn_free(gpointer data)
{
    hex_btn_t *h = data;
    if (h == NULL) return;
    gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(board_shell),
                                              TRUE);
    g_free(h->label);
    g_free(h);
    gtk_widget_set_margin_top(board_shell, 8);

static gboolean point_in_hexagon(double x, double y,
                                  double *pts, int n)
{
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

    state->shell = board_shell;
    double cx = width  / 2.0;
    state->base_width = 1670;
    state->base_height = 906;
    double cy = height / 2.0;
    double r  = (width < height ? width : height) * 0.50;
    double pts[12];
    state->cursor_valid = FALSE;
    double a0 = -M_PI / 2.0; /* pointy-top, matches the Figma SVG */
    for (int i = 0; i < 6; ++i) {
        double a = a0 + i * 2.0 * M_PI / 6.0;
        pts[2 * i]     = cx + r * cos(a);
        pts[2 * i + 1] = cy + r * sin(a);
    }

    cairo_save(cr);
    if (h->pressed)
        cairo_set_source_rgb(cr, 0.91, 0.53, 0.06); /* primary */
    else
        cairo_set_source_rgb(cr, 0.94, 0.92, 0.69); /* surface1 */
    cairo_move_to(cr, pts[0], pts[1]);
    for (int i = 1; i < 6; ++i) cairo_line_to(cr, pts[2*i], pts[2*i+1]);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0.85, 0.81, 0.63);
    cairo_set_line_width(cr, h->pressed ? 2.0 : 1.0);
    cairo_stroke(cr);

    PangoLayout *layout =
        gtk_widget_create_pango_layout(GTK_WIDGET(area), h->label);
    int lw, lh;
    pango_layout_get_pixel_size(layout, &lw, &lh);
    cairo_move_to(cr, cx - lw / 2.0, cy - lh / 2.0);
    if (h->label != NULL && h->label[0] != '\0') {
        cairo_set_source_rgb(cr, 0.18, 0.16, 0.11);
        pango_cairo_show_layout(cr, layout);
    }
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
    double r  = (w < ht ? w : ht) * 0.50;
    double pts[12];
    double a0 = -M_PI / 2.0;
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

    gtk_widget_set_tooltip_text(area, label != NULL && label[0] != '\0' ? label : "Agent cell");
    gtk_accessible_update_property(GTK_ACCESSIBLE(area),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   label != NULL && label[0] != '\0' ? label : "Agent cell",
                                   -1);
    return area;
}

/* ================================================================
 * Page builders
 * ================================================================ */

static GtkWidget *build_agents_page(void)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(page, "hive-agent-page");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);
    gtk_widget_set_halign(page, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(page, GTK_ALIGN_CENTER);

    

    GtkWidget *board_shell = gtk_scrolled_window_new();
    gtk_widget_add_css_class(board_shell, "hive-agent-board-shell");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(board_shell),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(board_shell),
                                              TRUE);
    gtk_widget_set_hexpand(board_shell, TRUE);
    gtk_widget_set_vexpand(board_shell, TRUE);
    gtk_widget_set_margin_top(board_shell, 33);
        state->hadj = g_object_ref(
            gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(board_shell)));
        state->vadj = g_object_ref(
            gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(board_shell)));
        g_object_set_data_full(G_OBJECT(board_shell), "agent-board-state",
                               state, agent_board_state_free);

        GtkEventController *motion_ctrl =
            GTK_EVENT_CONTROLLER(gtk_event_controller_motion_new());
        gtk_event_controller_set_propagation_phase(motion_ctrl, GTK_PHASE_CAPTURE);
        g_signal_connect(motion_ctrl, "motion",
                         G_CALLBACK(agent_board_record_cursor), state);
        gtk_widget_add_controller(board_shell, motion_ctrl);

        GtkGesture *pan_drag = gtk_gesture_drag_new();
        gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(pan_drag),
                                      GDK_BUTTON_MIDDLE);
        gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(pan_drag),
                                                   GTK_PHASE_CAPTURE);
        g_signal_connect(pan_drag, "drag-begin",
                         G_CALLBACK(agent_board_pan_begin), state);
        g_signal_connect(pan_drag, "drag-update",
                         G_CALLBACK(agent_board_pan_update), state);
        gtk_widget_add_controller(board_shell, GTK_EVENT_CONTROLLER(pan_drag));

    GtkWidget *board = gtk_fixed_new();
    gtk_widget_add_css_class(board, "hive-agent-board");
    gtk_widget_set_hexpand(board, TRUE);
    gtk_widget_set_vexpand(board, TRUE);

    const int hex_size = 100;
    const int cols = 15;
    const int rows = 10;
    const int step_x = 100;
    const int step_y = 86;
    const int board_w = 1500;
    const int board_h = 796;
    const int offset_x = (1740 - board_w) / 2;
    const int offset_y = (860 - board_h) / 2;

    int id = 1;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int px = offset_x + col * step_x + ((row % 2) ? 50 : 0);
            int py = offset_y + row * step_y;
            char label[32];
            snprintf(label, sizeof(label), "Agent %d", id++);
            GtkWidget *hb = create_hex_button("", hex_size);
            gtk_fixed_put(GTK_FIXED(board), hb, px, py);
            gtk_widget_set_tooltip_text(hb, label);
            gtk_accessible_update_property(GTK_ACCESSIBLE(hb),
                                           GTK_ACCESSIBLE_PROPERTY_LABEL,
                                           label, -1);
                // gtk_box_append(GTK_BOX(page), build_stats_strip());
    }

    gtk_box_append(GTK_BOX(board_shell), board);
    gtk_box_append(GTK_BOX(page), board_shell);
    return page;
}

#endif

#if 1
/* ================================================================
 * Page builders
 * ================================================================ */

/* ----------------------------------------------------------------
 * Role → fill colour mapping (RGB triples)
 * ---------------------------------------------------------------- */
typedef struct { double r, g, b; } rgb_t;

static rgb_t role_color(hive_agent_role_t role)
{
    switch (role) {
    case HIVE_ROLE_QUEEN:   return (rgb_t){0.91, 0.53, 0.06};  /* gold/amber  */
    case HIVE_ROLE_FORAGER: return (rgb_t){0.40, 0.73, 0.42};  /* green       */
    case HIVE_ROLE_BUILDER: return (rgb_t){0.38, 0.60, 0.85};  /* blue        */
    case HIVE_ROLE_NURSE:   return (rgb_t){0.78, 0.48, 0.78};  /* purple      */
    case HIVE_ROLE_GUARD:   return (rgb_t){0.85, 0.33, 0.31};  /* red         */
    case HIVE_ROLE_CLEANER: return (rgb_t){0.62, 0.62, 0.62};  /* grey        */
    case HIVE_ROLE_DRONE:   return (rgb_t){0.95, 0.77, 0.35};  /* pale yellow */
    default:                return (rgb_t){0.94, 0.92, 0.69};  /* surface1    */
    }
}

typedef struct {
    char *label;
    gboolean pressed;
    GtkWidget *area;
    hive_agent_role_t role;
    int agent_index;
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

    hive_agent_role_t role = h->role;
    if (h->agent_index >= 0 &&
        (size_t)h->agent_index < g_dynamics.agent_count) {
        role = g_dynamics.agents[h->agent_index].role;
    }

    double cx = width  / 2.0;
    double cy = height / 2.0;
    double r  = (width < height ? width : height) * 0.50;
    double pts[12];
    double a0 = -M_PI / 2.0;
    for (int i = 0; i < 6; ++i) {
        double a = a0 + i * 2.0 * M_PI / 6.0;
        pts[2 * i]     = cx + r * cos(a);
        pts[2 * i + 1] = cy + r * sin(a);
    }

    cairo_save(cr);

    /* Fill colour based on role */
    rgb_t fill;
    if (h->pressed) {
        fill = (rgb_t){0.91, 0.53, 0.06};
    } else {
        fill = role_color(role);
    }
    cairo_set_source_rgb(cr, fill.r, fill.g, fill.b);

    cairo_move_to(cr, pts[0], pts[1]);
    for (int i = 1; i < 6; ++i) cairo_line_to(cr, pts[2*i], pts[2*i+1]);
    cairo_close_path(cr);
    cairo_fill_preserve(cr);

    /* Border — slightly darker than fill */
    cairo_set_source_rgb(cr, fill.r * 0.75, fill.g * 0.75, fill.b * 0.75);
    cairo_set_line_width(cr, h->pressed ? 2.5 : 1.0);
    cairo_stroke(cr);

    /* Queen gets a pulsing glow ring */
    if (role == HIVE_ROLE_QUEEN) {
        cairo_set_source_rgba(cr, 0.91, 0.53, 0.06, 0.35);
        cairo_set_line_width(cr, 3.0);
        cairo_move_to(cr, pts[0], pts[1]);
        for (int i = 1; i < 6; ++i) cairo_line_to(cr, pts[2*i], pts[2*i+1]);
        cairo_close_path(cr);
        cairo_stroke(cr);
    }

    /* Badge character */
    char badge_str[2] = { hive_role_to_badge(role), '\0' };
    const char *display = (h->label != NULL && h->label[0] != '\0')
                          ? h->label : badge_str;

    PangoLayout *layout =
        gtk_widget_create_pango_layout(GTK_WIDGET(area), display);

    PangoFontDescription *fd = pango_font_description_new();
    pango_font_description_set_weight(fd, PANGO_WEIGHT_BOLD);
    if (role == HIVE_ROLE_QUEEN)
        pango_font_description_set_size(fd, 11 * PANGO_SCALE);
    else
        pango_font_description_set_size(fd, 9 * PANGO_SCALE);
    pango_layout_set_font_description(layout, fd);
    pango_font_description_free(fd);

    int lw, lh;
    pango_layout_get_pixel_size(layout, &lw, &lh);
    cairo_move_to(cr, cx - lw / 2.0, cy - lh / 2.0);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
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
    double r  = (w < ht ? w : ht) * 0.50;
    double pts[12];
    double a0 = -M_PI / 2.0;
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

static GtkWidget *create_hex_button(const char *label, int size,
                                    hive_agent_role_t role, int agent_idx)
{
    hex_btn_t *h = g_new0(hex_btn_t, 1);
    h->label   = g_strdup(label != NULL ? label : "");
    h->pressed = FALSE;
    h->role    = role;
    h->agent_index = agent_idx;

    GtkWidget *area = gtk_drawing_area_new();
    h->area = area;
    gtk_widget_set_size_request(area, size, size);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(area),
                                   hex_draw_fn, h, hex_btn_free);

    GtkGesture *g = gtk_gesture_click_new();
    g_signal_connect(g, "pressed", G_CALLBACK(hex_gesture_pressed), h);
    gtk_widget_add_controller(area, GTK_EVENT_CONTROLLER(g));

    char tip[64];
    snprintf(tip, sizeof(tip), "%s #%d", hive_role_to_string(role), agent_idx);
    gtk_widget_set_tooltip_text(area, tip);
    gtk_accessible_update_property(GTK_ACCESSIBLE(area),
                                   GTK_ACCESSIBLE_PROPERTY_LABEL,
                                   tip, -1);
    return area;
}

typedef struct {
    GtkWidget *widget;
    int agent_index;
    int base_x;
    int base_y;
    int base_size;
} agent_hex_cell_t;

typedef struct {
    GtkWidget *board;
    GtkWidget *shell;
    GtkAdjustment *hadj;
    GtkAdjustment *vadj;
    GPtrArray  *cells;
    int base_width;
    int base_height;
    double      zoom;
    double      min_zoom;
    double      max_zoom;
    double      cursor_x;
    double      cursor_y;
    gboolean    cursor_valid;
    double      pan_start_h;
    double      pan_start_v;
} agent_board_state_t;

static agent_board_state_t *g_agents_board_state = NULL;

static void agent_board_state_free(gpointer data)
{
    agent_board_state_t *state = data;
    if (state == NULL) return;
    if (g_agents_board_state == state)
        g_agents_board_state = NULL;
    if (state->hadj != NULL)
        g_object_unref(state->hadj);
    if (state->vadj != NULL)
        g_object_unref(state->vadj);
    if (state->cells != NULL)
        g_ptr_array_unref(state->cells);
    g_free(state);
}

static void agent_board_layout(agent_board_state_t *state)
{
    if (state == NULL || state->board == NULL || state->cells == NULL) return;

    gtk_widget_set_size_request(state->board,
                                (int)round(state->base_width * state->zoom),
                                (int)round(state->base_height * state->zoom));

    for (guint i = 0; i < state->cells->len; ++i) {
        agent_hex_cell_t *cell = g_ptr_array_index(state->cells, i);
        int size = (int)round(cell->base_size * state->zoom);
        int x = (int)round(cell->base_x * state->zoom);
        int y = (int)round(cell->base_y * state->zoom);
        if (size < 36) size = 36;
        gtk_widget_set_size_request(cell->widget, size, size);
        gtk_fixed_move(GTK_FIXED(state->board), cell->widget, x, y);
    }
}

static void agent_board_refresh(agent_board_state_t *state)
{
    if (state == NULL || state->cells == NULL) return;

    for (guint i = 0; i < state->cells->len; ++i) {
        agent_hex_cell_t *cell = g_ptr_array_index(state->cells, i);
        if (cell == NULL || cell->widget == NULL) continue;

        hive_agent_role_t role = HIVE_ROLE_EMPTY;
        if (cell->agent_index >= 0 &&
            (size_t)cell->agent_index < g_dynamics.agent_count) {
            role = g_dynamics.agents[cell->agent_index].role;
        }

        char tip[64];
        if (role == HIVE_ROLE_EMPTY) {
            snprintf(tip, sizeof(tip), "Empty slot #%d", cell->agent_index + 1);
        } else {
            snprintf(tip, sizeof(tip), "%s #%d",
                     hive_role_to_string(role), cell->agent_index + 1);
        }

        gtk_widget_set_tooltip_text(cell->widget, tip);
        gtk_accessible_update_property(GTK_ACCESSIBLE(cell->widget),
                                       GTK_ACCESSIBLE_PROPERTY_LABEL,
                                       tip, -1);
        gtk_widget_queue_draw(cell->widget);
    }
}

#define HIVE_INTAKE_MAX_BACKLOG 32

typedef struct {
    GtkWidget *window;
    GtkWidget *page;
    GtkWidget *prompt_view;
    GtkWidget *input_entry;
    GtkWidget *backlog_list;
    GtkWidget *backlog_count_label;
    GtkWidget *queen_status_label;
    GtkWidget *spawn_label;
    GtkWidget *signal_label;
    GtkWidget *status_label;
    GPtrArray *backlog_items;
} hive_intake_state_t;

static void intake_state_free(gpointer data)
{
    hive_intake_state_t *state = data;
    if (state == NULL) return;
    if (state->backlog_items != NULL)
        g_ptr_array_unref(state->backlog_items);
    g_free(state);
}

static char *text_view_dup(GtkWidget *text_view)
{
    if (text_view == NULL) return g_strdup("");

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
    GtkTextIter start;
    GtkTextIter end;
    gtk_text_buffer_get_bounds(buffer, &start, &end);
    char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
    if (text == NULL) return g_strdup("");
    return g_strstrip(text);
}

static void intake_refresh_inspector(const hive_intake_state_t *state)
{
    if (state == NULL || state->window == NULL) return;

    GtkWidget *intake_section =
        g_object_get_data(G_OBJECT(state->window), "intake-inspector-section");
    GtkWidget *signal_section =
        g_object_get_data(G_OBJECT(state->window), "signal-inspector-section");

    char buf[48];
    if (intake_section != NULL) {
        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.backlog_depth);
        hive_inspector_section_update_row(intake_section, "Backlog", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.queen_reviews);
        hive_inspector_section_update_row(intake_section, "Queen reviews", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.worker_spawns);
        hive_inspector_section_update_row(intake_section, "Worker spawns", buf);
    }

    if (signal_section != NULL) {
        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_pheromones);
        hive_inspector_section_update_row(signal_section, "Pheromones", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_waggles);
        hive_inspector_section_update_row(signal_section, "Waggles", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_alarms);
        hive_inspector_section_update_row(signal_section, "Alarms", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.pending_proposals);
        hive_inspector_section_update_row(signal_section, "Backlog proposals", buf);
    }
}

static void intake_refresh_metrics(const hive_intake_state_t *state)
{
    if (state == NULL) return;

    char buf[128];
    snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.backlog_depth);
    gtk_label_set_text(GTK_LABEL(state->backlog_count_label), buf);

    snprintf(buf, sizeof(buf), "Queen reviews: %u  |  Vitality: %u%%%s",
             g_dynamics.stats.queen_reviews,
             (unsigned)g_dynamics.vitality_checksum,
             g_dynamics.queen_alive ? "" : "  [!] re-queening");
    gtk_label_set_text(GTK_LABEL(state->queen_status_label), buf);

    snprintf(buf, sizeof(buf), "Worker spawns: %u  |  Active agents: %u",
             g_dynamics.stats.worker_spawns,
             g_dynamics.stats.total_agents);
    gtk_label_set_text(GTK_LABEL(state->spawn_label), buf);

    snprintf(buf, sizeof(buf), "Signals: %u  |  Pheromones: %u  |  Waggles: %u",
             g_dynamics.stats.total_signals,
             g_dynamics.stats.active_pheromones,
             g_dynamics.stats.active_waggles);
    gtk_label_set_text(GTK_LABEL(state->signal_label), buf);

    intake_refresh_inspector(state);
}

static void intake_set_status(hive_intake_state_t *state, const char *message)
{
    if (state == NULL) return;

    if (state->status_label != NULL) {
        gtk_label_set_text(GTK_LABEL(state->status_label),
                           message != NULL ? message : "");
    }

    if (state->window != NULL) {
        hive_main_window_set_status(state->window, message);
    }
}

static void intake_refresh_backlog_list(hive_intake_state_t *state)
{
    if (state == NULL || state->backlog_list == NULL || state->backlog_items == NULL)
        return;

    GtkWidget *child = gtk_widget_get_first_child(state->backlog_list);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_list_box_remove(GTK_LIST_BOX(state->backlog_list), child);
        child = next;
    }

    for (guint i = 0; i < state->backlog_items->len; ++i) {
        const char *item = g_ptr_array_index(state->backlog_items, i);
        GtkWidget *row_content = hive_session_row_new(item,
                                                      "Queued for queen review");
        GtkWidget *row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_content);
        gtk_list_box_append(GTK_LIST_BOX(state->backlog_list), row);
    }
}

static hive_agent_role_t intake_choose_worker_role(const char *text)
{
    char *lower = g_ascii_strdown(text != NULL ? text : "", -1);
    hive_agent_role_t role = HIVE_ROLE_FORAGER;

    if (lower == NULL) {
        return role;
    }

    if (strstr(lower, "drone") != NULL || strstr(lower, "experiment") != NULL) {
        role = HIVE_ROLE_DRONE;
    } else if (strstr(lower, "test") != NULL || strstr(lower, "verify") != NULL ||
               strstr(lower, "guard") != NULL) {
        role = HIVE_ROLE_GUARD;
    } else if (strstr(lower, "build") != NULL || strstr(lower, "implement") != NULL ||
               strstr(lower, "feature") != NULL) {
        role = HIVE_ROLE_BUILDER;
    } else if (strstr(lower, "clean") != NULL || strstr(lower, "fix") != NULL ||
               strstr(lower, "refactor") != NULL) {
        role = HIVE_ROLE_CLEANER;
    } else if (strstr(lower, "mentor") != NULL || strstr(lower, "review") != NULL ||
               strstr(lower, "nurse") != NULL) {
        role = HIVE_ROLE_NURSE;
    } else if (strstr(lower, "research") != NULL || strstr(lower, "docs") != NULL ||
               strstr(lower, "forage") != NULL) {
        role = HIVE_ROLE_FORAGER;
    }

    g_free(lower);
    return role;
}

static gboolean intake_spawn_worker(hive_agent_role_t role)
{
    if (g_dynamics.agent_count >= HIVE_DYNAMICS_MAX_AGENTS) {
        return FALSE;
    }

    /* Route all spawning through the Queen — she is the exclusive factory. */
    hive_agent_traits_t traits = {
        .temperature_pct  = 50U,
        .lineage_hash     = g_dynamics.lineage_generation,
        .specialization   = 0U,
        .resource_cap_pct = 80U,
    };
    size_t new_idx = hive_queen_spawn(&g_dynamics, role,
                                      hive_lifecycle_builtin_worker(), traits);
    if (new_idx == SIZE_MAX) {
        return FALSE;
    }

    hive_dynamics_recompute_stats(&g_dynamics);
    return TRUE;
}

static void intake_sync_model_from_backlog(hive_intake_state_t *state)
{
    if (state == NULL || state->backlog_items == NULL) return;

    /* Write into the canonical demand buffer so the Queen's regulation loop
     * can see it and auto-spawn when pressure is high. */
    g_dynamics.demand_buffer_depth  = (uint32_t)state->backlog_items->len;
    g_dynamics.stats.backlog_depth  = g_dynamics.demand_buffer_depth;
    g_dynamics.stats.pending_proposals = g_dynamics.demand_buffer_depth;
    g_dynamics.stats.quorum_threshold = g_dynamics.demand_buffer_depth > 0U
        ? g_dynamics.demand_buffer_depth
        : 1U;
    if (g_dynamics.stats.quorum_votes > g_dynamics.stats.quorum_threshold) {
        g_dynamics.stats.quorum_votes = g_dynamics.stats.quorum_threshold;
    }
}

static void intake_queue_prompt(hive_intake_state_t *state)
{
    if (state == NULL || state->backlog_items == NULL) return;

    if (state->backlog_items->len >= HIVE_INTAKE_MAX_BACKLOG) {
        intake_set_status(state, "Backlog full; queen is at intake capacity.");
        return;
    }

    char *prompt_text = text_view_dup(state->prompt_view);
    char *input_text = g_strdup(gtk_editable_get_text(GTK_EDITABLE(state->input_entry)));
    if (prompt_text == NULL || input_text == NULL) {
        g_free(prompt_text);
        g_free(input_text);
        intake_set_status(state, "Unable to copy the intake prompt.");
        return;
    }
    g_strstrip(prompt_text);
    g_strstrip(input_text);

    if ((prompt_text == NULL || prompt_text[0] == '\0') &&
        (input_text == NULL || input_text[0] == '\0')) {
        g_free(prompt_text);
        g_free(input_text);
        intake_set_status(state, "Enter a prompt or input before adding to the queen backlog.");
        return;
    }

    char *summary = NULL;
    if (prompt_text != NULL && prompt_text[0] != '\0' &&
        input_text != NULL && input_text[0] != '\0') {
        summary = g_strdup_printf("Prompt: %.96s | Input: %.96s", prompt_text, input_text);
    } else if (prompt_text != NULL && prompt_text[0] != '\0') {
        summary = g_strdup_printf("Prompt: %.96s", prompt_text);
    } else {
        summary = g_strdup_printf("Input: %.96s", input_text);
    }

    g_free(prompt_text);
    g_free(input_text);

    if (summary == NULL) {
        intake_set_status(state, "Unable to add backlog item.");
        return;
    }

    g_ptr_array_add(state->backlog_items, summary);
    intake_sync_model_from_backlog(state);
    intake_refresh_backlog_list(state);
    intake_refresh_metrics(state);
    gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->prompt_view)), "", -1);
    gtk_editable_set_text(GTK_EDITABLE(state->input_entry), "");

    char status[160];
    snprintf(status, sizeof(status), "Queued for queen review: %s", summary);
    intake_set_status(state, status);
}

static void intake_consult_queen(hive_intake_state_t *state)
{
    if (state == NULL || state->backlog_items == NULL) return;

    if (state->backlog_items->len == 0U) {
        intake_set_status(state, "Queen has no backlog to review yet.");
        return;
    }

    const char *summary = g_ptr_array_index(state->backlog_items, 0);
    hive_agent_role_t worker_role = intake_choose_worker_role(summary);

    if (!intake_spawn_worker(worker_role)) {
        intake_set_status(state, "Queen wants to spawn a worker, but the hive is at capacity.");
        return;
    }

    g_ptr_array_remove_index(state->backlog_items, 0);
    g_dynamics.demand_buffer_depth   = (uint32_t)state->backlog_items->len;
    g_dynamics.stats.backlog_depth   = g_dynamics.demand_buffer_depth;
    g_dynamics.stats.pending_proposals = g_dynamics.demand_buffer_depth;
    g_dynamics.stats.queen_reviews++;
    g_dynamics.stats.quorum_threshold = g_dynamics.demand_buffer_depth > 0U
        ? g_dynamics.demand_buffer_depth
        : 1U;
    if (g_dynamics.stats.quorum_votes > g_dynamics.stats.quorum_threshold) {
        g_dynamics.stats.quorum_votes = g_dynamics.stats.quorum_threshold;
    }
    hive_dynamics_recompute_stats(&g_dynamics);

    intake_refresh_backlog_list(state);
    intake_refresh_metrics(state);

    if (g_agents_board_state != NULL) {
        agent_board_refresh(g_agents_board_state);
    }

    char status[192];
    snprintf(status, sizeof(status), "Queen reviewed backlog and spawned a %s worker.",
             hive_role_to_string(worker_role));
    intake_set_status(state, status);
}

static void on_intake_queue_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    intake_queue_prompt((hive_intake_state_t *)user_data);
}

static void on_intake_consult_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    intake_consult_queen((hive_intake_state_t *)user_data);
}

static void agent_board_record_cursor(GtkEventControllerMotion *motion,
                                      double x,
                                      double y,
                                      gpointer user_data)
{
    (void)motion;
    agent_board_state_t *state = user_data;
    if (state == NULL) return;
    state->cursor_x = x;
    state->cursor_y = y;
    state->cursor_valid = TRUE;
}

static void agent_board_pan_begin(GtkGestureDrag *gesture,
                                  double start_x,
                                  double start_y,
                                  gpointer user_data)
{
    (void)gesture;
    (void)start_x;
    (void)start_y;
    agent_board_state_t *state = user_data;
    if (state == NULL) return;
    if (state->hadj != NULL)
        state->pan_start_h = gtk_adjustment_get_value(state->hadj);
    if (state->vadj != NULL)
        state->pan_start_v = gtk_adjustment_get_value(state->vadj);
}

static void agent_board_pan_update(GtkGestureDrag *gesture,
                                   double offset_x,
                                   double offset_y,
                                   gpointer user_data)
{
    (void)gesture;
    agent_board_state_t *state = user_data;
    if (state == NULL) return;
    if (state->hadj != NULL)
        gtk_adjustment_set_value(state->hadj, state->pan_start_h - offset_x);
    if (state->vadj != NULL)
        gtk_adjustment_set_value(state->vadj, state->pan_start_v - offset_y);
}

static void agent_board_zoom_at_cursor(agent_board_state_t *state,
                                       double factor)
{
    if (state == NULL || state->hadj == NULL || state->vadj == NULL)
        return;

    double old_zoom = state->zoom;
    double new_zoom = old_zoom * factor;
    if (new_zoom < state->min_zoom) new_zoom = state->min_zoom;
    if (new_zoom > state->max_zoom) new_zoom = state->max_zoom;
    if (fabs(new_zoom - old_zoom) < 0.0001)
        return;

    double cx = state->cursor_valid ? state->cursor_x
                                    : gtk_widget_get_width(state->shell) / 2.0;
    double cy = state->cursor_valid ? state->cursor_y
                                    : gtk_widget_get_height(state->shell) / 2.0;
    double hadj = gtk_adjustment_get_value(state->hadj);
    double vadj = gtk_adjustment_get_value(state->vadj);
    double anchor_x = (hadj + cx) / old_zoom;
    double anchor_y = (vadj + cy) / old_zoom;

    state->zoom = new_zoom;
    agent_board_layout(state);
    gtk_adjustment_set_value(state->hadj, anchor_x * new_zoom - cx);
    gtk_adjustment_set_value(state->vadj, anchor_y * new_zoom - cy);
}

static gboolean on_agent_board_scroll(GtkEventControllerScroll *controller,
                                      double dx,
                                      double dy,
                                      gpointer user_data)
{
    (void)dx;
    (void)controller;
    agent_board_state_t *state = user_data;
    if (state == NULL) return FALSE;

    if (dy < 0.0)
        agent_board_zoom_at_cursor(state, 1.10);
    else if (dy > 0.0)
        agent_board_zoom_at_cursor(state, 1.0 / 1.10);
    else
        return FALSE;

    return TRUE;
}

/* ----------------------------------------------------------------
 * Stats strip — role distribution + signal activity
 * ---------------------------------------------------------------- */

static GtkWidget *build_stats_strip(const hive_dynamics_t *dyn)
{
    GtkWidget *strip = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(strip, "hive-stats-strip");
    gtk_widget_set_hexpand(strip, TRUE);
    gtk_widget_set_margin_start(strip, 12);
    gtk_widget_set_margin_end(strip, 12);
    gtk_widget_set_margin_top(strip, 6);
    gtk_widget_set_margin_bottom(strip, 2);

    /* Role counts as colored badges */
    struct { hive_agent_role_t role; const char *css; } roles[] = {
        { HIVE_ROLE_QUEEN,   "hive-badge-queen"   },
        { HIVE_ROLE_FORAGER, "hive-badge-forager"  },
        { HIVE_ROLE_BUILDER, "hive-badge-builder"  },
        { HIVE_ROLE_NURSE,   "hive-badge-nurse"    },
        { HIVE_ROLE_GUARD,   "hive-badge-guard"    },
        { HIVE_ROLE_CLEANER, "hive-badge-cleaner"  },
        { HIVE_ROLE_DRONE,   "hive-badge-drone"    },
    };

    for (size_t i = 0; i < sizeof(roles) / sizeof(roles[0]); ++i) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%c %u",
                 hive_role_to_badge(roles[i].role),
                 dyn->stats.role_counts[roles[i].role]);
        GtkWidget *badge = gtk_label_new(buf);
        gtk_widget_add_css_class(badge, "hive-role-badge");
        gtk_widget_add_css_class(badge, roles[i].css);
        gtk_box_append(GTK_BOX(strip), badge);
    }

    /* Separator */
    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(strip), sep);

    /* Signal activity indicators */
    char sig_buf[64];
    snprintf(sig_buf, sizeof(sig_buf), "\xF0\x9F\x90\x9D %u agents",
             dyn->stats.total_agents);
    GtkWidget *agents_lbl = gtk_label_new(sig_buf);
    gtk_widget_add_css_class(agents_lbl, "hive-stats-label");
    gtk_box_append(GTK_BOX(strip), agents_lbl);

    snprintf(sig_buf, sizeof(sig_buf), "\xF0\x9F\x8C\xB8 %u pheromones",
             dyn->stats.active_pheromones);
    GtkWidget *phero_lbl = gtk_label_new(sig_buf);
    gtk_widget_add_css_class(phero_lbl, "hive-stats-label");
    gtk_box_append(GTK_BOX(strip), phero_lbl);

    snprintf(sig_buf, sizeof(sig_buf), "\xF0\x9F\x92\x83 %u waggles",
             dyn->stats.active_waggles);
    GtkWidget *waggle_lbl = gtk_label_new(sig_buf);
    gtk_widget_add_css_class(waggle_lbl, "hive-stats-label");
    gtk_box_append(GTK_BOX(strip), waggle_lbl);

    if (dyn->stats.active_alarms > 0) {
        snprintf(sig_buf, sizeof(sig_buf), "\xE2\x9A\xA0 %u alarms",
                 dyn->stats.active_alarms);
        GtkWidget *alarm_lbl = gtk_label_new(sig_buf);
        gtk_widget_add_css_class(alarm_lbl, "hive-stats-label");
        gtk_widget_add_css_class(alarm_lbl, "hive-stats-alarm");
        gtk_box_append(GTK_BOX(strip), alarm_lbl);
    }

    if (dyn->stats.pending_proposals > 0) {
        snprintf(sig_buf, sizeof(sig_buf), "\xF0\x9F\x97\xB3 %u proposals  (%u/%u quorum)",
                 dyn->stats.pending_proposals,
                 dyn->stats.quorum_votes,
                 dyn->stats.quorum_threshold);
        GtkWidget *quorum_lbl = gtk_label_new(sig_buf);
        gtk_widget_add_css_class(quorum_lbl, "hive-stats-label");
        gtk_box_append(GTK_BOX(strip), quorum_lbl);
    }

    return strip;
}

/* ----------------------------------------------------------------
 * Legend strip — role colour key
 * ---------------------------------------------------------------- */

static GtkWidget *build_legend_strip(void)
{
    GtkWidget *legend = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    gtk_widget_add_css_class(legend, "hive-legend-strip");
    gtk_widget_set_hexpand(legend, TRUE);
    gtk_widget_set_margin_start(legend, 12);
    gtk_widget_set_margin_end(legend, 12);
    gtk_widget_set_margin_bottom(legend, 4);

    struct { hive_agent_role_t role; const char *css; } items[] = {
        { HIVE_ROLE_QUEEN,   "hive-badge-queen"   },
        { HIVE_ROLE_FORAGER, "hive-badge-forager"  },
        { HIVE_ROLE_BUILDER, "hive-badge-builder"  },
        { HIVE_ROLE_NURSE,   "hive-badge-nurse"    },
        { HIVE_ROLE_GUARD,   "hive-badge-guard"    },
        { HIVE_ROLE_CLEANER, "hive-badge-cleaner"  },
        { HIVE_ROLE_DRONE,   "hive-badge-drone"    },
    };

    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

        GtkWidget *swatch = gtk_drawing_area_new();
        gtk_widget_set_size_request(swatch, 12, 12);
        gtk_widget_add_css_class(swatch, "hive-legend-swatch");
        gtk_widget_add_css_class(swatch, items[i].css);
        gtk_box_append(GTK_BOX(box), swatch);

        GtkWidget *lbl = gtk_label_new(hive_role_to_string(items[i].role));
        gtk_widget_add_css_class(lbl, "hive-legend-label");
        gtk_box_append(GTK_BOX(box), lbl);

        gtk_box_append(GTK_BOX(legend), box);
    }

    return legend;
}

static GtkWidget *build_agents_page(void)
{
    /* Ensure dynamics state is initialised */
    if (!g_dynamics_inited) {
        hive_dynamics_init(&g_dynamics, 1);
        g_dynamics_inited = TRUE;
    }

    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(page, "hive-agent-page");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);

    /* Stats strip at top */
    gtk_box_append(GTK_BOX(page), build_stats_strip(&g_dynamics));

    /* Legend strip */
    gtk_box_append(GTK_BOX(page), build_legend_strip());

    GtkWidget *board_shell = gtk_scrolled_window_new();
    gtk_widget_add_css_class(board_shell, "hive-agent-board-shell");
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(board_shell),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_kinetic_scrolling(GTK_SCROLLED_WINDOW(board_shell),
                                              TRUE);
    gtk_widget_set_hexpand(board_shell, TRUE);
    gtk_widget_set_vexpand(board_shell, TRUE);
    gtk_widget_set_margin_top(board_shell, 4);

    GtkWidget *board = gtk_fixed_new();
    gtk_widget_add_css_class(board, "hive-agent-board");
    gtk_widget_set_hexpand(board, TRUE);
    gtk_widget_set_vexpand(board, TRUE);

    const int hex_size = 100;
    const int cols = 15;
    const int rows = 10;
    const int step_x = 100;
    const int step_y = 86;
    const int board_w = 1500;
    const int board_h = 796;
    const int offset_x = (1740 - board_w) / 2;
    const int offset_y = (860 - board_h) / 2;

    agent_board_state_t *state = g_new0(agent_board_state_t, 1);
    state->board = board;
    state->shell = board_shell;
    state->cells = g_ptr_array_new_with_free_func(g_free);
    state->base_width = 1670;
    state->base_height = 906;
    state->zoom = 1.0;
    state->min_zoom = 0.5;
    state->max_zoom = 2.5;
    state->cursor_valid = FALSE;

    int id = 0;
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < cols; ++col) {
            int px = offset_x + col * step_x + ((row % 2) ? 50 : 0);
            int py = offset_y + row * step_y;

            /* Map cell to dynamics agent data */
            hive_agent_role_t role = HIVE_ROLE_EMPTY;
            if ((size_t)id < g_dynamics.agent_count)
                role = g_dynamics.agents[id].role;

            GtkWidget *hb = create_hex_button("", hex_size, role, id);
            gtk_fixed_put(GTK_FIXED(board), hb, px, py);

            agent_hex_cell_t *cell = g_new0(agent_hex_cell_t, 1);
            cell->widget = hb;
            cell->agent_index = id;
            cell->base_x = px;
            cell->base_y = py;
            cell->base_size = hex_size;
            g_ptr_array_add(state->cells, cell);

            id++;
        }
    }

    agent_board_layout(state);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(board_shell), board);
    state->hadj = g_object_ref(
        gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(board_shell)));
    state->vadj = g_object_ref(
        gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(board_shell)));
    g_agents_board_state = state;
    g_object_set_data_full(G_OBJECT(board_shell), "agent-board-state",
                           state, agent_board_state_free);

    GtkEventController *motion_ctrl =
        GTK_EVENT_CONTROLLER(gtk_event_controller_motion_new());
    gtk_event_controller_set_propagation_phase(motion_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect(motion_ctrl, "motion",
                     G_CALLBACK(agent_board_record_cursor), state);
    gtk_widget_add_controller(board_shell, motion_ctrl);

    GtkGesture *pan_drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(pan_drag),
                                  GDK_BUTTON_MIDDLE);
    gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(pan_drag),
                                               GTK_PHASE_CAPTURE);
    g_signal_connect(pan_drag, "drag-begin",
                     G_CALLBACK(agent_board_pan_begin), state);
    g_signal_connect(pan_drag, "drag-update",
                     G_CALLBACK(agent_board_pan_update), state);
    gtk_widget_add_controller(board_shell, GTK_EVENT_CONTROLLER(pan_drag));

    GtkEventController *scroll_ctrl =
        GTK_EVENT_CONTROLLER(gtk_event_controller_scroll_new(
            GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES));
    gtk_event_controller_set_propagation_phase(scroll_ctrl, GTK_PHASE_CAPTURE);
    g_signal_connect(scroll_ctrl, "scroll",
                     G_CALLBACK(on_agent_board_scroll), state);
    gtk_widget_add_controller(board_shell, scroll_ctrl);

    agent_board_refresh(state);

    gtk_box_append(GTK_BOX(page), board_shell);
    return page;
}

static GtkWidget *build_intake_page(void)
{
    if (!g_dynamics_inited) {
        hive_dynamics_init(&g_dynamics, 1);
        g_dynamics_inited = TRUE;
    }

    hive_intake_state_t *state = g_new0(hive_intake_state_t, 1);
    state->backlog_items = g_ptr_array_new_with_free_func(g_free);

    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(page, "hive-intake-page");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);
    gtk_widget_set_margin_top(page, 12);
    gtk_widget_set_margin_bottom(page, 12);
    gtk_widget_set_margin_start(page, 12);
    gtk_widget_set_margin_end(page, 12);

    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(hero, "hive-intake-hero");
    GtkWidget *title = gtk_label_new("Queen Intake");
    gtk_widget_add_css_class(title, "hive-intake-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_append(GTK_BOX(hero), title);

    GtkWidget *subtitle = gtk_label_new(
        "Prompts and input prompts land in the backlog. The queen reviews them, sends signals, and spawns workers.");
    gtk_widget_add_css_class(subtitle, "hive-intake-subtitle");
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
    gtk_box_append(GTK_BOX(hero), subtitle);
    gtk_box_append(GTK_BOX(page), hero);

    GtkWidget *metrics = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(metrics, "hive-intake-metrics");
    gtk_widget_set_hexpand(metrics, TRUE);

    state->backlog_count_label = gtk_label_new("0");
    state->queen_status_label = gtk_label_new("Queen reviews: 0");
    state->spawn_label = gtk_label_new("Worker spawns: 0 | Active agents: 1");
    state->signal_label = gtk_label_new("Signals: 0 | Pheromones: 0 | Waggles: 0");

    GtkWidget *metric_a = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *metric_b = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *metric_c = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *metric_d = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);

    GtkWidget *metric_a_label = gtk_label_new("Backlog");
    GtkWidget *metric_b_label = gtk_label_new("Queen");
    GtkWidget *metric_c_label = gtk_label_new("Workers");
    GtkWidget *metric_d_label = gtk_label_new("Signals");

    gtk_widget_add_css_class(metric_a_label, "hive-intake-metric-label");
    gtk_widget_add_css_class(metric_b_label, "hive-intake-metric-label");
    gtk_widget_add_css_class(metric_c_label, "hive-intake-metric-label");
    gtk_widget_add_css_class(metric_d_label, "hive-intake-metric-label");

    gtk_widget_add_css_class(state->backlog_count_label, "hive-intake-metric-value");
    gtk_widget_add_css_class(state->queen_status_label, "hive-intake-metric-value");
    gtk_widget_add_css_class(state->spawn_label, "hive-intake-metric-value");
    gtk_widget_add_css_class(state->signal_label, "hive-intake-metric-value");

    gtk_box_append(GTK_BOX(metric_a), metric_a_label);
    gtk_box_append(GTK_BOX(metric_a), state->backlog_count_label);
    gtk_box_append(GTK_BOX(metric_b), metric_b_label);
    gtk_box_append(GTK_BOX(metric_b), state->queen_status_label);
    gtk_box_append(GTK_BOX(metric_c), metric_c_label);
    gtk_box_append(GTK_BOX(metric_c), state->spawn_label);
    gtk_box_append(GTK_BOX(metric_d), metric_d_label);
    gtk_box_append(GTK_BOX(metric_d), state->signal_label);

    gtk_box_append(GTK_BOX(metrics), metric_a);
    gtk_box_append(GTK_BOX(metrics), metric_b);
    gtk_box_append(GTK_BOX(metrics), metric_c);
    gtk_box_append(GTK_BOX(metrics), metric_d);
    gtk_box_append(GTK_BOX(page), metrics);

    GtkWidget *input_card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_add_css_class(input_card, "hive-intake-card");
    gtk_widget_set_hexpand(input_card, TRUE);

    GtkWidget *prompt_label = gtk_label_new("Prompt");
    gtk_label_set_xalign(GTK_LABEL(prompt_label), 0.0);
    gtk_box_append(GTK_BOX(input_card), prompt_label);

    GtkWidget *prompt_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(prompt_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(prompt_scroll, "hive-intake-prompt-scroll");
    gtk_widget_set_vexpand(prompt_scroll, FALSE);
    gtk_widget_set_hexpand(prompt_scroll, TRUE);

    state->prompt_view = gtk_text_view_new();
    gtk_widget_add_css_class(state->prompt_view, "hive-intake-prompt-view");
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->prompt_view), GTK_WRAP_WORD_CHAR);
    gtk_widget_set_size_request(state->prompt_view, -1, 110);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(prompt_scroll),
                                  state->prompt_view);
    gtk_box_append(GTK_BOX(input_card), prompt_scroll);

    GtkWidget *input_label = gtk_label_new("Input prompt");
    gtk_label_set_xalign(GTK_LABEL(input_label), 0.0);
    gtk_box_append(GTK_BOX(input_card), input_label);

    state->input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(state->input_entry),
                                   "Optional context or constraints for the queen");
    gtk_widget_set_hexpand(state->input_entry, TRUE);
    gtk_box_append(GTK_BOX(input_card), state->input_entry);

    GtkWidget *button_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *queue_btn = gtk_button_new_with_label("Add to backlog");
    GtkWidget *review_btn = gtk_button_new_with_label("Consult queen");
    gtk_widget_add_css_class(review_btn, "suggested-action");
    g_signal_connect(queue_btn, "clicked",
                     G_CALLBACK(on_intake_queue_clicked), state);
    g_signal_connect(review_btn, "clicked",
                     G_CALLBACK(on_intake_consult_clicked), state);
    gtk_box_append(GTK_BOX(button_row), queue_btn);
    gtk_box_append(GTK_BOX(button_row), review_btn);
    gtk_box_append(GTK_BOX(input_card), button_row);
    gtk_box_append(GTK_BOX(page), input_card);

    GtkWidget *backlog_heading = gtk_label_new("Backlog");
    gtk_widget_add_css_class(backlog_heading, "hive-intake-section-title");
    gtk_label_set_xalign(GTK_LABEL(backlog_heading), 0.0);
    gtk_box_append(GTK_BOX(page), backlog_heading);

    GtkWidget *backlog_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(backlog_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(backlog_scroll, TRUE);
    gtk_widget_set_vexpand(backlog_scroll, TRUE);

    state->backlog_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->backlog_list),
                                    GTK_SELECTION_NONE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(backlog_scroll),
                                  state->backlog_list);
    gtk_box_append(GTK_BOX(page), backlog_scroll);

    state->status_label = gtk_label_new("Waiting for the first queen intake item.");
    gtk_widget_add_css_class(state->status_label, "hive-intake-status");
    gtk_label_set_wrap(GTK_LABEL(state->status_label), TRUE);
    gtk_label_set_xalign(GTK_LABEL(state->status_label), 0.0);
    gtk_box_append(GTK_BOX(page), state->status_label);

    state->page = page;
    g_object_set_data_full(G_OBJECT(page), "intake-state", state,
                           intake_state_free);

    intake_sync_model_from_backlog(state);
    intake_refresh_metrics(state);

    return page;
}

static void knowledge_append_row(GtkWidget *listbox,
                                 const char *title,
                                 const char *subtitle)
{
    if (listbox == NULL) return;

    GtkWidget *row_content = hive_session_row_new(title, subtitle);
    GtkWidget *row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_content);
    gtk_list_box_append(GTK_LIST_BOX(listbox), row);
}

static void knowledge_add_data_row(GtkWidget *grid,
                                   int row,
                                   const char *key,
                                   const char *value)
{
    if (grid == NULL || key == NULL) return;

    GtkWidget *key_label = gtk_label_new(key);
    gtk_widget_add_css_class(key_label, "hive-knowledge-key");
    gtk_label_set_xalign(GTK_LABEL(key_label), 1.0);
    gtk_label_set_wrap(GTK_LABEL(key_label), TRUE);
    gtk_widget_set_hexpand(key_label, FALSE);
    gtk_grid_attach(GTK_GRID(grid), key_label, 0, row, 1, 1);

    GtkWidget *value_label = gtk_label_new(value != NULL ? value : "—");
    gtk_widget_add_css_class(value_label, "hive-knowledge-value");
    gtk_label_set_xalign(GTK_LABEL(value_label), 0.0);
    gtk_label_set_wrap(GTK_LABEL(value_label), TRUE);
    gtk_widget_set_hexpand(value_label, TRUE);
    gtk_grid_attach(GTK_GRID(grid), value_label, 1, row, 1, 1);
}

static GtkWidget *build_knowledge_card(const char *title,
                                       const char *subtitle,
                                       GtkWidget *body)
{
    GtkWidget *card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(card, "hive-knowledge-card");
    gtk_widget_set_hexpand(card, TRUE);
    gtk_widget_set_vexpand(card, TRUE);

    GtkWidget *heading = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    GtkWidget *title_label = gtk_label_new(title != NULL ? title : "Knowledge");
    gtk_widget_add_css_class(title_label, "hive-knowledge-card-title");
    gtk_label_set_xalign(GTK_LABEL(title_label), 0.0);
    gtk_box_append(GTK_BOX(heading), title_label);

    GtkWidget *subtitle_label =
        gtk_label_new(subtitle != NULL ? subtitle : "");
    gtk_widget_add_css_class(subtitle_label, "hive-knowledge-card-subtitle");
    gtk_label_set_xalign(GTK_LABEL(subtitle_label), 0.0);
    gtk_label_set_wrap(GTK_LABEL(subtitle_label), TRUE);
    gtk_box_append(GTK_BOX(heading), subtitle_label);

    gtk_box_append(GTK_BOX(card), heading);
    if (body != NULL) {
        gtk_widget_set_hexpand(body, TRUE);
        gtk_widget_set_vexpand(body, TRUE);
        gtk_box_append(GTK_BOX(card), body);
    }

    return card;
}

static GtkWidget *build_knowledge_page(hive_runtime_t *runtime)
{
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(page, "hive-knowledge-page");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);
    gtk_widget_set_margin_top(page, 12);
    gtk_widget_set_margin_bottom(page, 12);
    gtk_widget_set_margin_start(page, 12);
    gtk_widget_set_margin_end(page, 12);

    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(hero, "hive-knowledge-hero");
    gtk_widget_set_hexpand(hero, TRUE);

    GtkWidget *title = gtk_label_new("Knowledge");
    gtk_widget_add_css_class(title, "hive-knowledge-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_append(GTK_BOX(hero), title);

    GtkWidget *subtitle = gtk_label_new(
        "Explore the graph, documents, and structured data available to the agents.");
    gtk_widget_add_css_class(subtitle, "hive-knowledge-subtitle");
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_box_append(GTK_BOX(hero), subtitle);
    gtk_box_append(GTK_BOX(page), hero);

    const char *graph_nodes[][2] = {
        { "Intake queue", "Prompts and input prompts waiting for queen review." },
        { "Queen review", "Decides which backlog items become work and signals." },
        { "Worker roles", "Cleaner, nurse, builder, guard, forager, and drone execution." },
        { "Signal bus", "Pheromones, waggle dances, alarms, and proposal broadcasts." },
        { "Document corpus", "Bee dynamics notes plus the dynamics prompt set." },
        { "Settings plumbing", "Runtime options, backend selection, and execution policy." },
    };

    const char *doc_entries[][2] = {
        { "docs/bees/dynamics.md", "Biological reference for hive roles, communication, and quorum." },
        { "docs/prompts/dynamics/00-overview.md", "Implementation overview and acceptance criteria." },
        { "docs/prompts/dynamics/01-role-mapping.md", "Queen / worker / drone lifecycle and task mapping." },
        { "docs/prompts/dynamics/02-architecture.md", "Signal subsystem, waggle dance, and quorum design." },
        { "docs/prompts/dynamics/03-pr-tasks.md", "Small PR breakdown for the dynamics work." },
        { "docs/prompts/dynamics/04-starter-code.md", "Queue and signal starter snippets." },
        { "docs/prompts/dynamics/05-tests-benchmarks.md", "Test and benchmark guidance for the hive subsystems." },
        { "docs/prompts/dynamics/06-spawn-lifecycle.md", "Spawn and retirement policy for queen-driven work allocation." },
    };

    const size_t graph_count = sizeof(graph_nodes) / sizeof(graph_nodes[0]);
    const size_t doc_count = sizeof(doc_entries) / sizeof(doc_entries[0]);
    const size_t data_count = 14U;

    GtkWidget *chips = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(chips, "hive-knowledge-chips");

    char chip_buf[64];
    snprintf(chip_buf, sizeof(chip_buf), "Graph nodes: %zu", graph_count);
    GtkWidget *graph_chip = gtk_label_new(chip_buf);
    gtk_widget_add_css_class(graph_chip, "hive-knowledge-chip");
    gtk_box_append(GTK_BOX(chips), graph_chip);

    snprintf(chip_buf, sizeof(chip_buf), "Documents: %zu", doc_count);
    GtkWidget *doc_chip = gtk_label_new(chip_buf);
    gtk_widget_add_css_class(doc_chip, "hive-knowledge-chip");
    gtk_box_append(GTK_BOX(chips), doc_chip);

    snprintf(chip_buf, sizeof(chip_buf), "Data fields: %zu", data_count);
    GtkWidget *data_chip = gtk_label_new(chip_buf);
    gtk_widget_add_css_class(data_chip, "hive-knowledge-chip");
    gtk_box_append(GTK_BOX(chips), data_chip);

    gtk_box_append(GTK_BOX(page), chips);

    GtkWidget *columns = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(columns, "hive-knowledge-columns");
    gtk_widget_set_hexpand(columns, TRUE);
    gtk_widget_set_vexpand(columns, TRUE);

    GtkWidget *graph_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(graph_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(graph_scroll, "hive-knowledge-scroll");
    gtk_widget_set_hexpand(graph_scroll, TRUE);
    gtk_widget_set_vexpand(graph_scroll, TRUE);

    GtkWidget *graph_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(graph_list), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(graph_list, "hive-knowledge-list");
    for (size_t i = 0; i < graph_count; ++i) {
        knowledge_append_row(graph_list, graph_nodes[i][0], graph_nodes[i][1]);
    }
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(graph_scroll), graph_list);

    GtkWidget *doc_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(doc_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(doc_scroll, "hive-knowledge-scroll");
    gtk_widget_set_hexpand(doc_scroll, TRUE);
    gtk_widget_set_vexpand(doc_scroll, TRUE);

    GtkWidget *doc_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(doc_list), GTK_SELECTION_NONE);
    gtk_widget_add_css_class(doc_list, "hive-knowledge-list");
    for (size_t i = 0; i < doc_count; ++i) {
        knowledge_append_row(doc_list, doc_entries[i][0], doc_entries[i][1]);
    }

    if (runtime != NULL) {
        knowledge_append_row(doc_list,
                             "User prompt",
                             runtime->session.user_prompt != NULL
                                 ? runtime->session.user_prompt
                                 : "No user prompt queued yet.");
        knowledge_append_row(doc_list,
                             "Project rules",
                             runtime->session.project_rules != NULL
                                 ? runtime->session.project_rules
                                 : "No project rules loaded.");
        knowledge_append_row(doc_list,
                             "Orchestrator brief",
                             runtime->session.orchestrator_brief != NULL
                                 ? runtime->session.orchestrator_brief
                                 : "No orchestrator brief yet.");
        knowledge_append_row(doc_list,
                             "Plan",
                             runtime->session.plan != NULL
                                 ? runtime->session.plan
                                 : "No plan generated yet.");
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(doc_scroll), doc_list);

    GtkWidget *data_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(data_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(data_scroll, "hive-knowledge-scroll");
    gtk_widget_set_hexpand(data_scroll, TRUE);
    gtk_widget_set_vexpand(data_scroll, TRUE);

    GtkWidget *data_grid = gtk_grid_new();
    gtk_widget_add_css_class(data_grid, "hive-knowledge-data-grid");
    gtk_grid_set_row_spacing(GTK_GRID(data_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(data_grid), 12);
    gtk_widget_set_margin_top(data_grid, 4);
    gtk_widget_set_margin_bottom(data_grid, 4);
    gtk_widget_set_margin_start(data_grid, 4);
    gtk_widget_set_margin_end(data_grid, 4);

    int row = 0;
    knowledge_add_data_row(data_grid, row++, "Workspace root",
                           runtime != NULL ? runtime->session.workspace_root : "—");
    knowledge_add_data_row(data_grid, row++, "User prompt",
                           runtime != NULL ? runtime->session.user_prompt : "—");
    knowledge_add_data_row(data_grid, row++, "Backend",
                           runtime != NULL && runtime->options.use_mock_inference
                               ? "mock"
                               : "configured");
    knowledge_add_data_row(data_grid, row++, "API bind",
                           runtime != NULL ? runtime->options.api_bind_address : "—");
    char number_buf[64];
    snprintf(number_buf, sizeof(number_buf), "%u",
             runtime != NULL ? runtime->options.api_port : 0U);
    knowledge_add_data_row(data_grid, row++, "API port", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u",
             runtime != NULL ? runtime->options.max_iterations : 0U);
    knowledge_add_data_row(data_grid, row++, "Max iterations", number_buf);
    knowledge_add_data_row(data_grid, row++, "Auto approve",
                           runtime != NULL && runtime->options.auto_approve ? "yes" : "no");
    snprintf(number_buf, sizeof(number_buf), "%u", g_dynamics.stats.backlog_depth);
    knowledge_add_data_row(data_grid, row++, "Backlog depth", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u", g_dynamics.stats.queen_reviews);
    knowledge_add_data_row(data_grid, row++, "Queen reviews", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u", g_dynamics.stats.worker_spawns);
    knowledge_add_data_row(data_grid, row++, "Worker spawns", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u", g_dynamics.stats.total_agents);
    knowledge_add_data_row(data_grid, row++, "Active agents", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u", g_dynamics.stats.total_signals);
    knowledge_add_data_row(data_grid, row++, "Total signals", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u / %u",
             g_dynamics.stats.quorum_votes,
             g_dynamics.stats.quorum_threshold);
    knowledge_add_data_row(data_grid, row++, "Quorum", number_buf);
    snprintf(number_buf, sizeof(number_buf), "%u / %s",
             (unsigned)g_dynamics.vitality_checksum,
             g_dynamics.queen_alive ? "alive" : "re-queening");
    knowledge_add_data_row(data_grid, row++, "Queen vitality", number_buf);

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(data_scroll), data_grid);

    gtk_box_append(GTK_BOX(columns),
                   build_knowledge_card("Graph",
                                        "Signal flow and role relationships available to the hive.",
                                        graph_scroll));
    gtk_box_append(GTK_BOX(columns),
                   build_knowledge_card("Documents",
                                        "Prompt corpus and live session documents the agents can inspect.",
                                        doc_scroll));
    gtk_box_append(GTK_BOX(columns),
                   build_knowledge_card("Data",
                                        "Live counters and runtime configuration surfaced to the agents.",
                                        data_scroll));

    gtk_box_append(GTK_BOX(page), columns);

    return page;
}

/* ================================================================
 * Templates page — lifecycle template browser & spawn
 * ================================================================ */

typedef struct {
    GtkWidget          *list;          /* GtkListBox — template entries */
    GtkWidget          *detail_name;   /* GtkLabel — selected template name */
    GtkWidget          *detail_desc;   /* GtkLabel — selected template description */
    GtkWidget          *stages_grid;   /* GtkGrid  — stage rows */
    GtkWidget          *spawn_btn;     /* GtkButton */
    GtkWidget          *spawn_spin;    /* GtkSpinButton — agent count */
    hive_lifecycle_id_t selected_id;
} hive_templates_state_t;

static void templates_state_free(gpointer data) { g_free(data); }

/* Role → CSS class (reuse existing badge classes) */
static const char *tmpl_role_css(hive_agent_role_t role)
{
    switch (role) {
    case HIVE_ROLE_QUEEN:   return "hive-badge-queen";
    case HIVE_ROLE_FORAGER: return "hive-badge-forager";
    case HIVE_ROLE_BUILDER: return "hive-badge-builder";
    case HIVE_ROLE_NURSE:   return "hive-badge-nurse";
    case HIVE_ROLE_GUARD:   return "hive-badge-guard";
    case HIVE_ROLE_CLEANER: return "hive-badge-cleaner";
    case HIVE_ROLE_DRONE:   return "hive-badge-drone";
    default:                return NULL;
    }
}

static void templates_refresh_stages(hive_templates_state_t *state,
                                     const hive_lifecycle_template_t *tmpl)
{
    GtkWidget *grid = state->stages_grid;

    /* Remove all existing children */
    GtkWidget *child = gtk_widget_get_first_child(grid);
    while (child != NULL) {
        GtkWidget *next = gtk_widget_get_next_sibling(child);
        gtk_grid_remove(GTK_GRID(grid), child);
        child = next;
    }

    if (tmpl == NULL || tmpl->stage_count == 0) return;

    /* Header row */
    const char *hdrs[] = { "Age window", "Role", "Firmness" };
    for (int c = 0; c < 3; ++c) {
        GtkWidget *lbl = gtk_label_new(hdrs[c]);
        gtk_widget_add_css_class(lbl, "hive-template-header");
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.0);
        gtk_grid_attach(GTK_GRID(grid), lbl, c, 0, 1, 1);
    }

    for (size_t i = 0; i < tmpl->stage_count; ++i) {
        const hive_lifecycle_stage_t *s = &tmpl->stages[i];
        int row = (int)i + 1;

        /* Age window */
        char age_buf[32];
        if (i + 1 < tmpl->stage_count)
            snprintf(age_buf, sizeof(age_buf), "ticks %u – %u",
                     s->start_tick, tmpl->stages[i + 1].start_tick - 1);
        else
            snprintf(age_buf, sizeof(age_buf), "tick %u +", s->start_tick);
        GtkWidget *age_lbl = gtk_label_new(age_buf);
        gtk_label_set_xalign(GTK_LABEL(age_lbl), 0.0);
        gtk_widget_add_css_class(age_lbl, "hive-template-stage-age");
        gtk_widget_set_margin_end(age_lbl, 8);
        gtk_grid_attach(GTK_GRID(grid), age_lbl, 0, row, 1, 1);

        /* Role badge */
        char badge_buf[32];
        snprintf(badge_buf, sizeof(badge_buf), "%c  %s",
                 hive_role_to_badge(s->role), hive_role_to_string(s->role));
        GtkWidget *role_lbl = gtk_label_new(badge_buf);
        gtk_label_set_xalign(GTK_LABEL(role_lbl), 0.0);
        gtk_widget_add_css_class(role_lbl, "hive-role-badge");
        const char *rc = tmpl_role_css(s->role);
        if (rc != NULL) gtk_widget_add_css_class(role_lbl, rc);
        gtk_widget_set_margin_end(role_lbl, 8);
        gtk_grid_attach(GTK_GRID(grid), role_lbl, 1, row, 1, 1);

        /* Firmness label */
        char firm_buf[32];
        if (s->firmness >= 100)
            snprintf(firm_buf, sizeof(firm_buf), "Forced");
        else
            snprintf(firm_buf, sizeof(firm_buf), "%u%% chance/tick", s->firmness);
        GtkWidget *firm_lbl = gtk_label_new(firm_buf);
        gtk_label_set_xalign(GTK_LABEL(firm_lbl), 0.0);
        gtk_widget_add_css_class(firm_lbl, "hive-template-stage-firmness");
        gtk_grid_attach(GTK_GRID(grid), firm_lbl, 2, row, 1, 1);
    }
}

static void on_template_row_selected(GtkListBox *list,
                                     GtkListBoxRow *row,
                                     gpointer user_data)
{
    (void)list;
    hive_templates_state_t *state = user_data;
    if (row == NULL) {
        state->selected_id = HIVE_LIFECYCLE_NONE;
        gtk_widget_set_sensitive(state->spawn_btn, FALSE);
        return;
    }

    hive_lifecycle_id_t id =
        (hive_lifecycle_id_t)(gintptr)g_object_get_data(G_OBJECT(row), "lifecycle-id");
    state->selected_id = id;

    const hive_lifecycle_template_t *tmpl = hive_lifecycle_get(id);
    if (tmpl == NULL) return;

    gtk_label_set_text(GTK_LABEL(state->detail_name), tmpl->name);
    gtk_label_set_text(GTK_LABEL(state->detail_desc), tmpl->description);
    gtk_widget_set_sensitive(state->spawn_btn, TRUE);

    templates_refresh_stages(state, tmpl);
}

static void on_template_spawn_clicked(GtkButton *btn, gpointer user_data)
{
    (void)btn;
    hive_templates_state_t *state = user_data;
    if (state->selected_id == HIVE_LIFECYCLE_NONE) return;

    if (!g_dynamics_inited) {
        hive_dynamics_init(&g_dynamics, 1);
        g_dynamics_inited = TRUE;
    }

    const hive_lifecycle_template_t *tmpl = hive_lifecycle_get(state->selected_id);
    if (tmpl == NULL) return;

    int count = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(state->spawn_spin));
    int spawned = 0;
    for (int i = 0; i < count; ++i) {
        if (g_dynamics.agent_count >= HIVE_DYNAMICS_MAX_AGENTS) break;
        hive_agent_cell_t *cell = &g_dynamics.agents[g_dynamics.agent_count];
        memset(cell, 0, sizeof(*cell));
        cell->lifecycle_id = state->selected_id;
        cell->role         = hive_role_for_age(0, tmpl);
        cell->age_ticks    = 0U;
        cell->perf_score   = 50U;
        cell->signal_count = 1U;
        g_dynamics.agent_count++;
        g_dynamics.stats.worker_spawns++;
        spawned++;
    }
    hive_dynamics_recompute_stats(&g_dynamics);

    if (g_agents_board_state != NULL)
        agent_board_refresh(g_agents_board_state);

    char msg[128];
    snprintf(msg, sizeof(msg), "Spawned %d %s agent%s.",
             spawned, tmpl->name, spawned == 1 ? "" : "s");

    /* Re-use the window status bar if available via parent walk */
    GtkWidget *w = GTK_WIDGET(gtk_widget_get_root(state->list));
    if (w != NULL)
        hive_main_window_set_status(w, msg);
}

static GtkWidget *build_templates_page(void)
{
    if (!g_dynamics_inited) {
        hive_dynamics_init(&g_dynamics, 1);
        g_dynamics_inited = TRUE;
    }

    hive_templates_state_t *state = g_new0(hive_templates_state_t, 1);
    state->selected_id = HIVE_LIFECYCLE_NONE;

    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(page, "hive-templates-page");
    gtk_widget_set_hexpand(page, TRUE);
    gtk_widget_set_vexpand(page, TRUE);
    gtk_widget_set_margin_top(page, 12);
    gtk_widget_set_margin_bottom(page, 12);
    gtk_widget_set_margin_start(page, 12);
    gtk_widget_set_margin_end(page, 12);

    g_object_set_data_full(G_OBJECT(page), "templates-state", state,
                           templates_state_free);

    /* ---- Hero ---- */
    GtkWidget *hero = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(hero, "hive-templates-hero");

    GtkWidget *title = gtk_label_new("Lifecycle Templates");
    gtk_widget_add_css_class(title, "hive-templates-title");
    gtk_label_set_xalign(GTK_LABEL(title), 0.0);
    gtk_box_append(GTK_BOX(hero), title);

    GtkWidget *subtitle = gtk_label_new(
        "Agents age through fixed role stages — like bees. "
        "Select a template to inspect its stage ladder, then spawn agents that will "
        "accumulate knowledge in each role before progressing to the next.");
    gtk_widget_add_css_class(subtitle, "hive-templates-subtitle");
    gtk_label_set_wrap(GTK_LABEL(subtitle), TRUE);
    gtk_label_set_xalign(GTK_LABEL(subtitle), 0.0);
    gtk_box_append(GTK_BOX(hero), subtitle);
    gtk_box_append(GTK_BOX(page), hero);

    /* ---- Two-column pane ---- */
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_paned_set_position(GTK_PANED(paned), 260);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_resize_end_child(GTK_PANED(paned), TRUE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_end_child(GTK_PANED(paned), FALSE);
    gtk_widget_set_hexpand(paned, TRUE);
    gtk_widget_set_vexpand(paned, TRUE);

    /* ---- Left: template list ---- */
    GtkWidget *list_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(list_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_add_css_class(list_scroll, "hive-templates-list-scroll");
    gtk_widget_set_vexpand(list_scroll, TRUE);

    state->list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state->list), GTK_SELECTION_SINGLE);
    gtk_widget_add_css_class(state->list, "hive-templates-list");
    g_signal_connect(state->list, "row-selected",
                     G_CALLBACK(on_template_row_selected), state);

    size_t n = hive_lifecycle_count();
    for (size_t i = 0; i < n; ++i) {
        hive_lifecycle_id_t id = (hive_lifecycle_id_t)(i + 1);
        const hive_lifecycle_template_t *tmpl = hive_lifecycle_get(id);
        if (tmpl == NULL) continue;

        GtkWidget *row_content = hive_session_row_new(tmpl->name, tmpl->description);
        GtkWidget *row = gtk_list_box_row_new();
        gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), row_content);
        g_object_set_data(G_OBJECT(row), "lifecycle-id", (gpointer)(gintptr)id);
        gtk_list_box_append(GTK_LIST_BOX(state->list), row);
    }

    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(list_scroll), state->list);
    gtk_paned_set_start_child(GTK_PANED(paned), list_scroll);

    /* ---- Right: detail panel ---- */
    GtkWidget *detail = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_add_css_class(detail, "hive-templates-detail");
    gtk_widget_set_hexpand(detail, TRUE);
    gtk_widget_set_vexpand(detail, TRUE);
    gtk_widget_set_margin_start(detail, 12);

    state->detail_name = gtk_label_new("Select a template");
    gtk_widget_add_css_class(state->detail_name, "hive-templates-detail-name");
    gtk_label_set_xalign(GTK_LABEL(state->detail_name), 0.0);
    gtk_box_append(GTK_BOX(detail), state->detail_name);

    state->detail_desc = gtk_label_new("");
    gtk_widget_add_css_class(state->detail_desc, "hive-templates-detail-desc");
    gtk_label_set_wrap(GTK_LABEL(state->detail_desc), TRUE);
    gtk_label_set_xalign(GTK_LABEL(state->detail_desc), 0.0);
    gtk_box_append(GTK_BOX(detail), state->detail_desc);

    GtkWidget *stages_heading = gtk_label_new("Stage ladder");
    gtk_widget_add_css_class(stages_heading, "hive-templates-section-title");
    gtk_label_set_xalign(GTK_LABEL(stages_heading), 0.0);
    gtk_box_append(GTK_BOX(detail), stages_heading);

    GtkWidget *stages_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(stages_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(stages_scroll, TRUE);
    gtk_widget_set_vexpand(stages_scroll, TRUE);

    state->stages_grid = gtk_grid_new();
    gtk_widget_add_css_class(state->stages_grid, "hive-templates-stages-grid");
    gtk_grid_set_row_spacing(GTK_GRID(state->stages_grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(state->stages_grid), 16);
    gtk_widget_set_margin_top(state->stages_grid, 4);
    gtk_widget_set_margin_bottom(state->stages_grid, 4);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(stages_scroll), state->stages_grid);
    gtk_box_append(GTK_BOX(detail), stages_scroll);

    /* ---- Spawn controls ---- */
    GtkWidget *spawn_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(spawn_row, "hive-templates-spawn-row");
    gtk_widget_set_hexpand(spawn_row, TRUE);

    GtkWidget *spawn_label = gtk_label_new("Spawn");
    gtk_box_append(GTK_BOX(spawn_row), spawn_label);

    GtkAdjustment *adj = gtk_adjustment_new(1.0, 1.0, 16.0, 1.0, 4.0, 0.0);
    state->spawn_spin = gtk_spin_button_new(adj, 1.0, 0);
    gtk_widget_set_size_request(state->spawn_spin, 72, -1);
    gtk_box_append(GTK_BOX(spawn_row), state->spawn_spin);

    GtkWidget *spawn_label2 = gtk_label_new("agent(s) with this template");
    gtk_widget_set_hexpand(spawn_label2, TRUE);
    gtk_label_set_xalign(GTK_LABEL(spawn_label2), 0.0);
    gtk_box_append(GTK_BOX(spawn_row), spawn_label2);

    state->spawn_btn = gtk_button_new_with_label("Spawn");
    gtk_widget_add_css_class(state->spawn_btn, "suggested-action");
    gtk_widget_set_sensitive(state->spawn_btn, FALSE);
    g_signal_connect(state->spawn_btn, "clicked",
                     G_CALLBACK(on_template_spawn_clicked), state);
    gtk_box_append(GTK_BOX(spawn_row), state->spawn_btn);

    gtk_box_append(GTK_BOX(detail), spawn_row);

    gtk_paned_set_end_child(GTK_PANED(paned), detail);
    gtk_box_append(GTK_BOX(page), paned);

    /* Auto-select the first template if any exist */
    GtkListBoxRow *first = gtk_list_box_get_row_at_index(
        GTK_LIST_BOX(state->list), 0);
    if (first != NULL)
        gtk_list_box_select_row(GTK_LIST_BOX(state->list), first);

    return page;
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

#endif

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

static void on_bottom_nav_clicked(GtkButton *btn, gpointer user_data)
{
    GtkStack *stack = GTK_STACK(user_data);
    const char *page = g_object_get_data(G_OBJECT(btn), "page-name");
    if (page != NULL)
        gtk_stack_set_visible_child_name(stack, page);

    GPtrArray *btns = g_object_get_data(G_OBJECT(stack), "bottom-nav-buttons");
    if (btns != NULL) {
        for (guint i = 0; i < btns->len; ++i) {
            GtkWidget *b = g_ptr_array_index(btns, i);
            if (b == GTK_WIDGET(btn))
                gtk_widget_add_css_class(b, "nav-active");
            else
                gtk_widget_remove_css_class(b, "nav-active");
        }
    }
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
    gtk_window_set_default_size(GTK_WINDOW(window), 1600, 960);
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

    GtkWidget *intake_page = build_intake_page();
    hive_intake_state_t *intake_state =
        g_object_get_data(G_OBJECT(intake_page), "intake-state");
    if (intake_state != NULL) {
        intake_state->window = window;
    }

    gtk_stack_add_named(GTK_STACK(stack),
                        intake_page, "intake");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_agents_page(),   "agents");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_templates_page(), "templates");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_knowledge_page(runtime),   "knowledge");
    gtk_stack_add_named(GTK_STACK(stack),
                        build_settings_page(), "settings");
    gtk_stack_set_visible_child_name(GTK_STACK(stack), "intake");

    /* (Left sidebar nav removed — replaced by a bottom nav bar below)
     * The bottom nav is created later and wired to the same stack. */

    /* ---- Inspector ---- */
    GtkWidget *inspector = hive_inspector_pane_new();
    gtk_widget_add_css_class(inspector, "hive-session-panel");
    gtk_widget_set_size_request(inspector, 240, -1);
    GtkWidget *intake_section =
        hive_inspector_pane_add_section(inspector, "Intake");
    hive_inspector_section_add_row(intake_section, "Backlog", "0");
    hive_inspector_section_add_row(intake_section, "Queen reviews", "0");
    hive_inspector_section_add_row(intake_section, "Worker spawns", "0");

    GtkWidget *runtime_section =
        hive_inspector_pane_add_section(inspector, "Runtime");
    hive_inspector_section_add_row(runtime_section, "Mode",
                                    runtime != NULL ? "live" : "no runtime");
    hive_inspector_section_add_row(runtime_section, "Mock LLM",
                                    runtime != NULL &&
                                    runtime->options.use_mock_inference
                                    ? "yes" : "no");

    /* ---- Dynamics section in inspector ---- */
    {
        char buf[32];
        GtkWidget *dyn_section =
            hive_inspector_pane_add_section(inspector, "Hive Dynamics");

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.total_agents);
        hive_inspector_section_add_row(dyn_section, "Agents", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_QUEEN]);
        hive_inspector_section_add_row(dyn_section, "Queens", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_FORAGER]);
        hive_inspector_section_add_row(dyn_section, "Foragers", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_BUILDER]);
        hive_inspector_section_add_row(dyn_section, "Builders", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_NURSE]);
        hive_inspector_section_add_row(dyn_section, "Nurses", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_GUARD]);
        hive_inspector_section_add_row(dyn_section, "Guards", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_CLEANER]);
        hive_inspector_section_add_row(dyn_section, "Cleaners", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.role_counts[HIVE_ROLE_DRONE]);
        hive_inspector_section_add_row(dyn_section, "Drones", buf);

        GtkWidget *sig_section =
            hive_inspector_pane_add_section(inspector, "Signals");

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_pheromones);
        hive_inspector_section_add_row(sig_section, "Pheromones", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_waggles);
        hive_inspector_section_add_row(sig_section, "Waggles", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.active_alarms);
        hive_inspector_section_add_row(sig_section, "Alarms", buf);

        snprintf(buf, sizeof(buf), "%u", g_dynamics.stats.pending_proposals);
        hive_inspector_section_add_row(sig_section, "Backlog proposals", buf);

        snprintf(buf, sizeof(buf), "%u / %u",
                 g_dynamics.stats.quorum_votes,
                 g_dynamics.stats.quorum_threshold);
        hive_inspector_section_add_row(sig_section, "Quorum", buf);

        g_object_set_data(G_OBJECT(window), "intake-inspector-section",
                          intake_section);
        g_object_set_data(G_OBJECT(window), "signal-inspector-section",
                          sig_section);
    }

    if (intake_state != NULL) {
        intake_refresh_metrics(intake_state);
    }

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
    gtk_paned_set_position(GTK_PANED(inner), 1360);
    gtk_widget_set_hexpand(inner, TRUE);
    gtk_widget_set_vexpand(inner, TRUE);
    /* ---- Bottom nav (tabs) ---- */
    GtkWidget *bottom_nav = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(bottom_nav, "hive-bottom-nav");
    gtk_widget_set_hexpand(bottom_nav, TRUE);

    GPtrArray *nav_buttons = g_ptr_array_new();
    g_object_set_data_full(G_OBJECT(stack), "bottom-nav-buttons",
                           nav_buttons, (GDestroyNotify)g_ptr_array_unref);

    struct { const char *icon; const char *label; const char *page; } nav_items[] = {
        { "edit-paste-symbolic",          "Intake",     "intake"    },
        { "applications-science-symbolic","Agents",     "agents"    },
        { "emblem-system-symbolic",       "Templates",  "templates" },
        { "folder-documents-symbolic",    "Knowledge",  "knowledge" },
        { "preferences-system-symbolic",  "Settings",   "settings"  },
        { NULL, NULL, NULL }
    };

    for (int i = 0; nav_items[i].label != NULL; ++i) {
        GtkWidget *btn = gtk_button_new();
        gtk_widget_add_css_class(btn, "hive-bottom-nav-item");
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        if (nav_items[i].icon) {
            GtkWidget *icon = gtk_image_new_from_icon_name(nav_items[i].icon);
            gtk_image_set_pixel_size(GTK_IMAGE(icon), 16);
            gtk_box_append(GTK_BOX(hbox), icon);
        }
        GtkWidget *lbl = gtk_label_new(nav_items[i].label);
        gtk_label_set_xalign(GTK_LABEL(lbl), 0.5);
        gtk_box_append(GTK_BOX(hbox), lbl);
        gtk_button_set_child(GTK_BUTTON(btn), hbox);
        g_object_set_data_full(G_OBJECT(btn), "page-name",
                               g_strdup(nav_items[i].page), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_bottom_nav_clicked), stack);
        gtk_box_append(GTK_BOX(bottom_nav), btn);
        g_ptr_array_add(nav_buttons, btn);
    }

    /* Select default (Intake) */
    if (nav_buttons->len > 0) {
        GtkWidget *def = g_ptr_array_index(nav_buttons, 0);
        on_bottom_nav_clicked(GTK_BUTTON(def), stack);
    }

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
    gtk_box_append(GTK_BOX(content), statusbar);
    gtk_box_append(GTK_BOX(content), search_bar);
    gtk_box_append(GTK_BOX(content), inner);
    gtk_box_append(GTK_BOX(content), bottom_nav);
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
