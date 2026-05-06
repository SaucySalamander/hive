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
 * Helpers
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

static void draw_stats_panel(hive_runtime_t *runtime, int y, int x, int height, int width)
{
    hive_dynamics_t *d = &runtime->dynamics;

    draw_box(y, x, height, width);
    mvprintw(y + 1, x + 2, " QUEUE STATS ");
    mvprintw(y + 2, x + 2, "Demand depth:     %5u", hive_dynamics_get_demand_depth(d));
    mvprintw(y + 3, x + 2, "Active workers:   %5u", d->stats.role_counts[HIVE_ROLE_CLEANER] +
                                                     d->stats.role_counts[HIVE_ROLE_NURSE] +
                                                     d->stats.role_counts[HIVE_ROLE_BUILDER] +
                                                     d->stats.role_counts[HIVE_ROLE_GUARD] +
                                                     d->stats.role_counts[HIVE_ROLE_FORAGER]);
    mvprintw(y + 4, x + 2, "Waggle strength:  %3u%%", d->stats.waggle_strength);
    mvprintw(y + 5, x + 2, "Waggle duration:  %5u ticks", d->stats.waggle_duration);
    mvprintw(y + 6, x + 2, "Total agents:     %5u", d->stats.total_agents);
    mvprintw(y + 7, x + 2, "Active alarms:    %5u", d->stats.active_alarms);
}

static void draw_plan_input_panel(hive_tui_plan_context_t *ctx, int y, int x, int height, int width)
{
    draw_box(y, x, height, width);
    mvprintw(y + 1, x + 2, " CREATE PLAN ");

    int field_y = y + 2;
    
    /* Task description field */
    attr_t attr = (ctx->edit_field == 0) ? A_REVERSE : A_NORMAL;
    attron(attr);
    mvprintw(field_y, x + 2, "Task: ");
    mvprintw(field_y, x + 10, "%-40s", ctx->task_description);
    attroff(attr);
    
    /* Model override field */
    field_y += 2;
    attr = (ctx->edit_field == 1) ? A_REVERSE : A_NORMAL;
    attron(attr);
    mvprintw(field_y, x + 2, "Model (optional): ");
    mvprintw(field_y, x + 22, "%-28s", ctx->model_override);
    attroff(attr);
    
    /* Role hints field */
    field_y += 2;
    attr = (ctx->edit_field == 2) ? A_REVERSE : A_NORMAL;
    attron(attr);
    mvprintw(field_y, x + 2, "Role hints: ");
    mvprintw(field_y, x + 15, "%-45s", ctx->role_hints);
    attroff(attr);
}

static void draw_review_panel(hive_tui_plan_context_t *ctx, int y, int x, int height, int width)
{
    draw_box(y, x, height, width);
    mvprintw(y + 1, x + 2, " REVIEW PLAN ");
    
    if (strlen(ctx->task_description) == 0) {
        mvprintw(y + 3, x + 2, "No plan to review yet.");
        return;
    }

    mvprintw(y + 2, x + 2, "Task: %s", ctx->task_description);
    if (strlen(ctx->model_override) > 0)
        mvprintw(y + 3, x + 2, "Model: %s", ctx->model_override);
    if (strlen(ctx->role_hints) > 0)
        mvprintw(y + 4, x + 2, "Roles: %s", ctx->role_hints);
    
    mvprintw(y + 6, x + 2, "Pending work units: %u", ctx->pending_demand);
    mvprintw(y + 8, x + 2, "Press ENTER to approve, ESC to cancel");
}

static void draw_menu(hive_tui_plan_context_t *ctx)
{
    int menu_y = LINES - 3;
    attron(A_BOLD);
    mvprintw(menu_y, 0, "=== PLAN MODE ===");
    attroff(A_BOLD);
    
    mvprintw(menu_y + 1, 0, "[TAB] Edit | [R] Review | [A] Approve | [Q] Quit");
    if (ctx->edit_field < 3)
        mvprintw(menu_y + 1, 50, "Editing field %d/3", ctx->edit_field + 1);
}

/* ================================================================
 * Edit field input handler
 * ================================================================ */

static void edit_field_text(char *buffer, size_t max_len, const char *prompt __attribute__((unused)))
{
    int x = getcurx(stdscr);
    int y = getcury(stdscr);
    
    size_t len = strlen(buffer);
    int pos = (int)len;
    
    curs_set(1);  /* Show cursor */
    
    while (true) {
        mvprintw(y, x, "%-50s", buffer);
        mvprintw(y, x + pos, "%c", ' ');
        refresh();
        
        int key = getch();
        
        if (key == '\t' || key == KEY_DOWN) {
            break;  /* Move to next field */
        } else if (key == '\n' || key == KEY_UP) {
            break;  /* Move to previous field */
        } else if (key == 27) {  /* ESC */
            break;
        } else if (key == KEY_BACKSPACE || key == 8 || key == 127) {
            if (pos > 0) {
                pos--;
                buffer[pos] = '\0';
            }
        } else if (isprint(key) && pos < (int)max_len - 1) {
            buffer[pos++] = (char)key;
            buffer[pos] = '\0';
        }
    }
    
    curs_set(0);  /* Hide cursor */
}

/* ================================================================
 * Main plan mode UI loop
 * ================================================================ */

hive_status_t hive_tui_page_queen(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    hive_tui_plan_context_t ctx = {
        .task_description = {0},
        .model_override = {0},
        .role_hints = {0},
        .pending_demand = 0,
        .edit_field = 0,
    };

    bool in_review = false;

    while (true) {
        erase();
        
        /* Left column: input/review */
        if (!in_review) {
            draw_plan_input_panel(&ctx, 1, 1, 10, COLS / 2 - 2);
        } else {
            draw_review_panel(&ctx, 1, 1, 12, COLS / 2 - 2);
        }
        
        /* Right column: queue stats */
        draw_stats_panel(runtime, 1, COLS / 2, LINES - 5, COLS / 2 - 1);
        
        draw_menu(&ctx);
        
        refresh();
        
        int key = getch();
        
        if (key == 'q' || key == 'Q' || key == 27) {
            break;  /* Quit plan mode */
        } else if (key == 'r' || key == 'R') {
            in_review = true;
            ctx.pending_demand = strlen(ctx.task_description) > 0 ? 100 : 0;
        } else if (key == 'a' || key == 'A') {
            if (in_review && ctx.pending_demand > 0) {
                /* Approve and inject demand */
                hive_status_t status = hive_dynamics_inject_demand(&runtime->dynamics, ctx.pending_demand);
                if (status == HIVE_STATUS_OK) {
                    /* Show confirmation */
                    erase();
                    attron(A_BOLD);
                    mvprintw(LINES / 2, (COLS - 30) / 2, "PLAN APPROVED");
                    attroff(A_BOLD);
                    mvprintw(LINES / 2 + 1, (COLS - 40) / 2, "Injected %u work units into demand buffer", ctx.pending_demand);
                    mvprintw(LINES / 2 + 2, (COLS - 25) / 2, "New demand depth: %u", hive_dynamics_get_demand_depth(&runtime->dynamics));
                    mvprintw(LINES / 2 + 4, (COLS - 20) / 2, "Press any key to continue");
                    refresh();
                    getch();
                    
                    /* Reset context for next plan */
                    memset(&ctx, 0, sizeof(ctx));
                    in_review = false;
                } else {
                    /* Show error */
                    erase();
                    attron(A_BOLD);
                    mvprintw(LINES / 2, (COLS - 30) / 2, "ERROR");
                    attroff(A_BOLD);
                    mvprintw(LINES / 2 + 1, (COLS - 30) / 2, "Failed to inject demand");
                    mvprintw(LINES / 2 + 3, (COLS - 20) / 2, "Press any key to continue");
                    refresh();
                    getch();
                    in_review = false;
                }
            }
        } else if (key == '\t' || key == KEY_DOWN) {
            /* Move to next field */
            if (!in_review) {
                ctx.edit_field = (ctx.edit_field + 1) % 3;
            }
        } else if (key == KEY_UP) {
            /* Move to previous field */
            if (!in_review) {
                ctx.edit_field = (ctx.edit_field == 0) ? 2 : ctx.edit_field - 1;
            }
        } else if (key == '\n' && !in_review) {
            /* Edit the current field */
            if (ctx.edit_field == 0) {
                edit_field_text(ctx.task_description, sizeof(ctx.task_description), "Task: ");
            } else if (ctx.edit_field == 1) {
                edit_field_text(ctx.model_override, sizeof(ctx.model_override), "Model: ");
            } else if (ctx.edit_field == 2) {
                edit_field_text(ctx.role_hints, sizeof(ctx.role_hints), "Roles: ");
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
