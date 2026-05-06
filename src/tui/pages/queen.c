#include "queen.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif

#include "core/dynamics/buffer_injection.h"
#include "core/dynamics/dynamics.h"
#include <string.h>
#include <ctype.h>

/* ================================================================
 * Visual line helpers for word-wrapped multiline display
 * ================================================================ */

typedef struct {
    int start;       /* byte offset in buffer where line begins */
    int len;         /* number of chars displayed (excludes newline) */
    bool hard_break; /* true if terminated by an explicit '\n' */
} vis_line_t;

#define MAX_VIS_LINES 1024

static int compute_vis_lines(const char *text, int width,
                              vis_line_t *out, int max_out)
{
    int n = 0;
    int i = 0;
    int tlen = (int)strlen(text);

    while (n < max_out) {
        out[n].start = i;
        int col = 0;
        while (i < tlen && text[i] != '\n' && col < width) {
            i++;
            col++;
        }
        out[n].len = col;
        out[n].hard_break = (i < tlen && text[i] == '\n');
        n++;
        if (i < tlen && text[i] == '\n') i++;
        if (i >= tlen) break;
    }
    if (n == 0) {
        out[0] = (vis_line_t){0, 0, false};
        n = 1;
    }
    return n;
}

/* Map a byte offset to its (visual line, column) */
static void offset_to_vis(const vis_line_t *lines, int nlines, int offset,
                           int *out_vl, int *out_vc)
{
    for (int l = 0; l < nlines; l++) {
        int line_end = lines[l].start + lines[l].len;
        bool last = (l == nlines - 1);
        if (offset <= line_end || last) {
            *out_vl = l;
            *out_vc = offset - lines[l].start;
            if (*out_vc < 0) *out_vc = 0;
            if (*out_vc > lines[l].len) *out_vc = lines[l].len;
            return;
        }
    }
    *out_vl = 0;
    *out_vc = 0;
}

/* ================================================================
 * Task area renderer — draws word-wrapped text with optional cursor
 * ================================================================ */

static void draw_task_area(int y, int x, int rows, int cols,
                            const char *text, int cursor, int scroll,
                            bool show_cursor)
{
    vis_line_t lines[MAX_VIS_LINES];
    int nlines = compute_vis_lines(text, cols, lines, MAX_VIS_LINES);

    int cur_vl = 0, cur_vc = 0;
    if (show_cursor)
        offset_to_vis(lines, nlines, cursor, &cur_vl, &cur_vc);

    for (int r = 0; r < rows; r++) {
        int l = r + scroll;
        mvhline(y + r, x, ' ', cols);
        if (l >= nlines) continue;

        const char *line_start = text + lines[l].start;
        int line_len = lines[l].len;

        if (show_cursor && l == cur_vl) {
            if (cur_vc > 0)
                mvaddnstr(y + r, x, line_start, cur_vc);
            int ch = (cursor < (int)strlen(text)) ? (unsigned char)text[cursor] : ' ';
            if (ch == '\n') ch = ' ';
            attron(A_REVERSE);
            mvaddch(y + r, x + cur_vc, ch);
            attroff(A_REVERSE);
            if (cur_vc < line_len - 1)
                mvaddnstr(y + r, x + cur_vc + 1,
                          line_start + cur_vc + 1, line_len - cur_vc - 1);
        } else {
            mvaddnstr(y + r, x, line_start, line_len);
        }
    }
}

/* ================================================================
 * Layout geometry — single source of truth for panel positions
 * ================================================================ */

typedef struct {
    int panel_y, panel_x, panel_h, panel_w;
    int task_area_y, task_area_x, task_rows, task_cols;
    int model_y,  model_x,  model_w;
    int roles_y,  roles_x,  roles_w;
} plan_layout_t;

static plan_layout_t compute_layout(void)
{
    plan_layout_t L;
    L.panel_y = 1;
    L.panel_x = 1;
    L.panel_h = LINES - 5;
    L.panel_w = COLS / 2 - 2;

    /* Task area starts at row 3 inside panel; leave 6 rows below for fields */
    L.task_rows = L.panel_h - 8;
    if (L.task_rows < 4) L.task_rows = 4;
    L.task_area_y = L.panel_y + 3;
    L.task_area_x = L.panel_x + 2;
    L.task_cols   = L.panel_w - 4;

    /* Model field below task area */
    L.model_y = L.task_area_y + L.task_rows + 1;
    L.model_x = L.panel_x + 20;
    L.model_w = L.panel_w - 22;

    /* Roles field below model */
    L.roles_y = L.model_y + 2;
    L.roles_x = L.panel_x + 14;
    L.roles_w = L.panel_w - 16;

    return L;
}

/* ================================================================
 * Inline multiline task editor — cursor lives inside the task area
 * ================================================================ */

static void edit_task_inline(hive_tui_plan_context_t *ctx, const plan_layout_t *L)
{
    char *buf      = ctx->task_description;
    size_t buf_cap = sizeof(ctx->task_description);

    while (true) {
        vis_line_t lines[MAX_VIS_LINES];
        int nlines = compute_vis_lines(buf, L->task_cols, lines, MAX_VIS_LINES);
        int len = (int)strlen(buf);

        int cur_vl, cur_vc;
        offset_to_vis(lines, nlines, ctx->task_cursor, &cur_vl, &cur_vc);

        /* Auto-scroll so cursor stays visible */
        if (cur_vl < ctx->task_scroll)
            ctx->task_scroll = cur_vl;
        if (cur_vl >= ctx->task_scroll + L->task_rows)
            ctx->task_scroll = cur_vl - L->task_rows + 1;

        draw_task_area(L->task_area_y, L->task_area_x,
                       L->task_rows, L->task_cols,
                       buf, ctx->task_cursor, ctx->task_scroll, true);

        /* Status hint while editing */
        mvprintw(LINES - 2, 0, "Editing spec — ESC to stop | arrows to move | Enter for newline");
        mvhline(LINES - 2, 56, ' ', COLS - 56);

        refresh();

        int key = getch();

        if (key == 27) break; /* ESC: leave editing mode */

        if (key == KEY_BACKSPACE || key == 8 || key == 127) {
            if (ctx->task_cursor > 0) {
                memmove(buf + ctx->task_cursor - 1,
                        buf + ctx->task_cursor,
                        (size_t)(len - ctx->task_cursor + 1));
                ctx->task_cursor--;
            }
        } else if (key == '\n' || key == KEY_ENTER) {
            if (len + 1 < (int)buf_cap - 1) {
                memmove(buf + ctx->task_cursor + 1,
                        buf + ctx->task_cursor,
                        (size_t)(len - ctx->task_cursor + 1));
                buf[ctx->task_cursor++] = '\n';
            }
        } else if (key == KEY_LEFT) {
            if (ctx->task_cursor > 0) ctx->task_cursor--;
        } else if (key == KEY_RIGHT) {
            if (ctx->task_cursor < len) ctx->task_cursor++;
        } else if (key == KEY_UP) {
            if (cur_vl > 0) {
                int tc = (cur_vc <= lines[cur_vl - 1].len) ? cur_vc : lines[cur_vl - 1].len;
                ctx->task_cursor = lines[cur_vl - 1].start + tc;
            }
        } else if (key == KEY_DOWN) {
            if (cur_vl < nlines - 1) {
                int tc = (cur_vc <= lines[cur_vl + 1].len) ? cur_vc : lines[cur_vl + 1].len;
                ctx->task_cursor = lines[cur_vl + 1].start + tc;
            }
        } else if (key == KEY_HOME) {
            ctx->task_cursor = lines[cur_vl].start;
        } else if (key == KEY_END) {
            ctx->task_cursor = lines[cur_vl].start + lines[cur_vl].len;
        } else if (key == KEY_DC) {
            if (ctx->task_cursor < len) {
                memmove(buf + ctx->task_cursor,
                        buf + ctx->task_cursor + 1,
                        (size_t)(len - ctx->task_cursor));
            }
        } else if (isprint(key) && len + 1 < (int)buf_cap - 1) {
            memmove(buf + ctx->task_cursor + 1,
                    buf + ctx->task_cursor,
                    (size_t)(len - ctx->task_cursor + 1));
            buf[ctx->task_cursor++] = (char)key;
        }
    }
}

/* ================================================================
 * Single-line field editor — cursor always in the field, not bottom
 * ================================================================ */

static void edit_single_line_at(char *buffer, size_t max_len,
                                 int y, int x, int field_width)
{
    int len = (int)strlen(buffer);
    int pos = len;

    curs_set(1);

    while (true) {
        mvhline(y, x, ' ', field_width);
        mvaddnstr(y, x, buffer, len);
        int disp_ch = (pos < len) ? (unsigned char)buffer[pos] : ' ';
        attron(A_REVERSE);
        mvaddch(y, x + pos, disp_ch);
        attroff(A_REVERSE);
        move(y, x + pos);
        refresh();

        int key = getch();

        if (key == 27 || key == '\t' || key == KEY_UP || key == KEY_DOWN) break;
        if (key == '\n' || key == KEY_ENTER) break;

        if (key == KEY_BACKSPACE || key == 8 || key == 127) {
            if (pos > 0) {
                memmove(buffer + pos - 1, buffer + pos, (size_t)(len - pos + 1));
                pos--;
                len--;
            }
        } else if (key == KEY_LEFT) {
            if (pos > 0) pos--;
        } else if (key == KEY_RIGHT) {
            if (pos < len) pos++;
        } else if (key == KEY_HOME) {
            pos = 0;
        } else if (key == KEY_END) {
            pos = len;
        } else if (key == KEY_DC) {
            if (pos < len) {
                memmove(buffer + pos, buffer + pos + 1, (size_t)(len - pos));
                len--;
                buffer[len] = '\0';
            }
        } else if (isprint(key) && len + 1 < (int)max_len - 1) {
            memmove(buffer + pos + 1, buffer + pos, (size_t)(len - pos + 1));
            buffer[pos++] = (char)key;
            len++;
        }
    }

    curs_set(0);
}

/* ================================================================
 * Panel drawing
 * ================================================================ */

static void draw_box(int y, int x, int height, int width)
{
    for (int i = 0; i < width; i++) {
        mvaddch(y, x + i, ACS_HLINE);
        mvaddch(y + height - 1, x + i, ACS_HLINE);
    }
    for (int i = 0; i < height; i++) {
        mvaddch(y + i, x, ACS_VLINE);
        mvaddch(y + i, x + width - 1, ACS_VLINE);
    }
    mvaddch(y, x, ACS_ULCORNER);
    mvaddch(y, x + width - 1, ACS_URCORNER);
    mvaddch(y + height - 1, x, ACS_LLCORNER);
    mvaddch(y + height - 1, x + width - 1, ACS_LRCORNER);
}

static void draw_plan_input_panel(hive_tui_plan_context_t *ctx,
                                   const plan_layout_t *L)
{
    draw_box(L->panel_y, L->panel_x, L->panel_h, L->panel_w);
    mvprintw(L->panel_y + 1, L->panel_x + 2, " CREATE PLAN ");

    /* Spec label — bold when field is selected */
    bool spec_sel = (ctx->edit_field == 0);
    if (spec_sel) attron(A_BOLD | A_UNDERLINE);
    mvprintw(L->panel_y + 2, L->panel_x + 2, "Spec (ENTER to edit, ESC to stop):");
    if (spec_sel) attroff(A_BOLD | A_UNDERLINE);

    /* Task text rendered inside the panel */
    draw_task_area(L->task_area_y, L->task_area_x,
                   L->task_rows, L->task_cols,
                   ctx->task_description, ctx->task_cursor,
                   ctx->task_scroll, false);

    /* Scroll indicator */
    vis_line_t lines[MAX_VIS_LINES];
    int nlines = compute_vis_lines(ctx->task_description, L->task_cols,
                                   lines, MAX_VIS_LINES);
    if (nlines > L->task_rows) {
        mvprintw(L->task_area_y + L->task_rows - 1,
                 L->panel_x + L->panel_w - 10,
                 "[+%d]", nlines - ctx->task_scroll - L->task_rows);
    }

    /* Model field */
    attr_t attr = (ctx->edit_field == 1) ? A_REVERSE : A_NORMAL;
    attron(attr);
    mvprintw(L->model_y, L->panel_x + 2, "Model (optional): ");
    mvprintw(L->model_y, L->model_x, "%-*s", L->model_w, ctx->model_override);
    attroff(attr);

    /* Roles field */
    attr = (ctx->edit_field == 2) ? A_REVERSE : A_NORMAL;
    attron(attr);
    mvprintw(L->roles_y, L->panel_x + 2, "Role hints:   ");
    mvprintw(L->roles_y, L->roles_x, "%-*s", L->roles_w, ctx->role_hints);
    attroff(attr);
}

static void draw_stats_panel(hive_runtime_t *runtime, int y, int x,
                              int height, int width)
{
    hive_dynamics_t *d = &runtime->dynamics;

    draw_box(y, x, height, width);
    mvprintw(y + 1, x + 2, " QUEUE STATS ");
    mvprintw(y + 2, x + 2, "Demand depth:     %5u", hive_dynamics_get_demand_depth(d));
    mvprintw(y + 3, x + 2, "Active workers:   %5u",
             d->stats.role_counts[HIVE_ROLE_CLEANER] +
             d->stats.role_counts[HIVE_ROLE_NURSE]   +
             d->stats.role_counts[HIVE_ROLE_BUILDER] +
             d->stats.role_counts[HIVE_ROLE_GUARD]   +
             d->stats.role_counts[HIVE_ROLE_FORAGER]);
    mvprintw(y + 4, x + 2, "Waggle strength:  %3u%%", d->stats.waggle_strength);
    mvprintw(y + 5, x + 2, "Waggle duration:  %5u ticks", d->stats.waggle_duration);
    mvprintw(y + 6, x + 2, "Total agents:     %5u", d->stats.total_agents);
    mvprintw(y + 7, x + 2, "Active alarms:    %5u", d->stats.active_alarms);
}

static void draw_review_panel(hive_tui_plan_context_t *ctx, int y, int x,
                               int height, int width)
{
    draw_box(y, x, height, width);
    mvprintw(y + 1, x + 2, " REVIEW PLAN ");

    if (strlen(ctx->task_description) == 0) {
        mvprintw(y + 3, x + 2, "No spec written yet — press TAB/ENTER to edit.");
        return;
    }

    int area_cols = width - 4;
    vis_line_t lines[MAX_VIS_LINES];
    int nlines = compute_vis_lines(ctx->task_description, area_cols, lines, MAX_VIS_LINES);
    int show_rows = height - 9;
    if (show_rows < 2) show_rows = 2;

    mvprintw(y + 2, x + 2, "Spec:");
    for (int l = 0; l < nlines && l < show_rows; l++)
        mvaddnstr(y + 3 + l, x + 4,
                  ctx->task_description + lines[l].start, lines[l].len);
    if (nlines > show_rows)
        mvprintw(y + 3 + show_rows, x + 4, "... (%d more lines)", nlines - show_rows);

    int info_y = y + 4 + show_rows;
    if (strlen(ctx->model_override) > 0)
        mvprintw(info_y++, x + 2, "Model: %s", ctx->model_override);
    if (strlen(ctx->role_hints) > 0)
        mvprintw(info_y++, x + 2, "Roles: %s", ctx->role_hints);

    mvprintw(y + height - 3, x + 2, "Work units: %u", ctx->pending_demand);
    mvprintw(y + height - 2, x + 2, "ENTER=approve  ESC=back");
}

static void draw_menu(hive_tui_plan_context_t *ctx, bool in_review)
{
    int menu_y = LINES - 3;
    attron(A_BOLD);
    mvprintw(menu_y, 0, "=== PLAN MODE ===");
    attroff(A_BOLD);

    if (in_review) {
        mvprintw(menu_y + 1, 0,
                 "[ENTER] Approve  [ESC] Back  [Q] Quit plan mode");
    } else {
        const char *field_names[] = {"Task Spec", "Model", "Role Hints"};
        const char *name = (ctx->edit_field >= 0 && ctx->edit_field < 3)
                           ? field_names[ctx->edit_field] : "?";
        mvprintw(menu_y + 1, 0,
                 "[TAB] Next field  [ENTER] Edit  [R] Review  [Q] Quit  — Field: %s",
                 name);
    }
}

/* ================================================================
 * Main plan mode loop
 * ================================================================ */

hive_status_t hive_tui_page_queen(hive_runtime_t *runtime)
{
    if (runtime == NULL)
        return HIVE_STATUS_INVALID_ARGUMENT;

    hive_tui_plan_context_t ctx = {
        .task_description = {0},
        .model_override   = {0},
        .role_hints       = {0},
        .pending_demand   = 0,
        .edit_field       = 0,
        .task_cursor      = 0,
        .task_scroll      = 0,
    };

    bool in_review = false;

    while (true) {
        plan_layout_t L = compute_layout();
        erase();

        if (!in_review) {
            draw_plan_input_panel(&ctx, &L);
        } else {
            draw_review_panel(&ctx, 1, 1, LINES - 5, COLS / 2 - 2);
        }
        draw_stats_panel(runtime, 1, COLS / 2, LINES - 5, COLS / 2 - 1);
        draw_menu(&ctx, in_review);
        refresh();

        int key = getch();

        if (key == 'q' || key == 'Q') {
            break;
        } else if (key == 27) {
            if (in_review) in_review = false;
            else break;
        } else if (key == 'r' || key == 'R') {
            if (!in_review) {
                in_review = true;
                ctx.pending_demand = strlen(ctx.task_description) > 0 ? 100 : 0;
            }
        } else if ((key == '\n' || key == 'a' || key == 'A') && in_review) {
            if (ctx.pending_demand > 0) {
                hive_status_t st = hive_dynamics_inject_demand(&runtime->dynamics,
                                                               ctx.pending_demand);
                erase();
                if (st == HIVE_STATUS_OK) {
                    attron(A_BOLD);
                    mvprintw(LINES / 2, (COLS - 14) / 2, "PLAN APPROVED");
                    attroff(A_BOLD);
                    mvprintw(LINES / 2 + 1, (COLS - 40) / 2,
                             "Injected %u work units into demand buffer",
                             ctx.pending_demand);
                    mvprintw(LINES / 2 + 2, (COLS - 28) / 2,
                             "New demand depth: %u",
                             hive_dynamics_get_demand_depth(&runtime->dynamics));
                    mvprintw(LINES / 2 + 4, (COLS - 22) / 2, "Press any key to continue");
                    refresh();
                    getch();
                    memset(&ctx, 0, sizeof(ctx));
                    in_review = false;
                } else {
                    attron(A_BOLD);
                    mvprintw(LINES / 2, (COLS - 14) / 2, "INJECT FAILED");
                    attroff(A_BOLD);
                    mvprintw(LINES / 2 + 2, (COLS - 14) / 2, "Press any key");
                    refresh();
                    getch();
                    in_review = false;
                }
            }
        } else if (!in_review) {
            if (key == '\t' || key == KEY_DOWN) {
                ctx.edit_field = (ctx.edit_field + 1) % 3;
            } else if (key == KEY_UP) {
                ctx.edit_field = (ctx.edit_field == 0) ? 2 : ctx.edit_field - 1;
            } else if (key == '\n') {
                plan_layout_t el = compute_layout();
                if (ctx.edit_field == 0) {
                    edit_task_inline(&ctx, &el);
                } else if (ctx.edit_field == 1) {
                    edit_single_line_at(ctx.model_override,
                                        sizeof(ctx.model_override),
                                        el.model_y, el.model_x, el.model_w);
                } else if (ctx.edit_field == 2) {
                    edit_single_line_at(ctx.role_hints,
                                        sizeof(ctx.role_hints),
                                        el.roles_y, el.roles_x, el.roles_w);
                }
            }
        }
    }

    return HIVE_STATUS_OK;
}

#else  /* !HIVE_HAVE_NCURSES */

hive_status_t hive_tui_page_queen(hive_runtime_t *runtime)
{
    (void)runtime;
    return HIVE_STATUS_UNAVAILABLE;
}

#endif  /* HIVE_HAVE_NCURSES */
