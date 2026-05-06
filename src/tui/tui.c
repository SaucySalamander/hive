#include "tui/tui.h"
#include "tui/pages/queen.h"
#include "tui/pages/dashboard.h"
#include "tui/pages/backlog.h"
#include "tui/pages/status.h"
#include "tui/pages/settings.h"

#include "core/runtime.h"
#include "common/logging/logger.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* ================================================================
 * Background run state — shared between run thread and TUI
 * ================================================================ */

typedef struct {
    pthread_t       thread;
    pthread_mutex_t mutex;
    pthread_cond_t  approval_cond;
    bool            started;          /* pthread_create was called */
    bool            active;           /* run in progress */
    bool            complete;         /* run finished */
    hive_status_t   result;
    bool            approval_pending; /* run thread awaiting decision */
    bool            approval_result;
    char            approval_tool[256];
    char            approval_details[2048];
} hive_tui_bg_run_t;

typedef struct hive_tui_context {
    hive_runtime_t   *runtime;
    hive_tui_bg_run_t run;
} hive_tui_context_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

/* ================================================================
 * Approval callback — called from run thread, blocks until resolved
 * ================================================================ */

static bool approval_callback(void *user_data, const char *tool_name,
                               const char *details)
{
    hive_tui_context_t *ctx = user_data;
    hive_tui_bg_run_t  *r   = &ctx->run;

    pthread_mutex_lock(&r->mutex);
    snprintf(r->approval_tool,    sizeof(r->approval_tool),    "%s", safe_text(tool_name));
    snprintf(r->approval_details, sizeof(r->approval_details), "%s", safe_text(details));
    r->approval_pending = true;
    while (r->approval_pending)
        pthread_cond_wait(&r->approval_cond, &r->mutex);
    bool result = r->approval_result;
    pthread_mutex_unlock(&r->mutex);

    if (ctx->runtime->logger.initialized) {
        hive_logger_log(&ctx->runtime->logger,
                        result ? HIVE_LOG_INFO : HIVE_LOG_WARN,
                        "tui", "approval", result ? "approved" : "denied");
    }
    return result;
}

/* ================================================================
 * Run thread — stderr suppressed to avoid corrupting the ncurses display
 * ================================================================ */

static void *bg_run_thread(void *arg)
{
    hive_tui_context_t *ctx = arg;
    hive_tui_bg_run_t  *r   = &ctx->run;

    /* Redirect stderr so structured log lines don't corrupt ncurses */
    int saved_stderr = dup(STDERR_FILENO);
    int dev_null     = open("/dev/null", O_WRONLY);
    if (dev_null >= 0) {
        dup2(dev_null, STDERR_FILENO);
        close(dev_null);
    }

    r->result = hive_runtime_run(ctx->runtime);

    if (saved_stderr >= 0) {
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
    }

    pthread_mutex_lock(&r->mutex);
    r->active   = false;
    r->complete = true;
    pthread_mutex_unlock(&r->mutex);
    return NULL;
}

/* ================================================================
 * Resolve pending approval (main thread only)
 * ================================================================ */

static void handle_pending_approval(hive_tui_context_t *ctx)
{
    hive_tui_bg_run_t *r = &ctx->run;

    pthread_mutex_lock(&r->mutex);
    char tool[256], details[2048];
    snprintf(tool,    sizeof(tool),    "%s", r->approval_tool);
    snprintf(details, sizeof(details), "%s", r->approval_details);
    pthread_mutex_unlock(&r->mutex);

    erase();
    attron(A_BOLD);
    mvprintw(0, 0, "hive — tool approval required");
    attroff(A_BOLD);
    mvprintw(2, 0, "Tool:    %s", tool);
    mvprintw(4, 0, "Request:");
    mvaddnstr(5, 2, details, COLS - 2);
    mvprintw(LINES - 2, 0, "[Y] Approve  [N] Deny");
    refresh();

    bool approved = false;
    while (true) {
        int key = getch();
        if (key == ERR) continue;
        if (key == 'y' || key == 'Y') { approved = true; break; }
        if (key == 'n' || key == 'N' || key == 27) break;
    }

    pthread_mutex_lock(&r->mutex);
    r->approval_result  = approved;
    r->approval_pending = false;
    pthread_cond_signal(&r->approval_cond);
    pthread_mutex_unlock(&r->mutex);
}

/* ================================================================
 * Run status page — live view, approval handling, navigation allowed
 * ================================================================ */

static void show_run_status_page(hive_tui_context_t *ctx)
{
    hive_tui_bg_run_t *r  = &ctx->run;
    hive_runtime_t    *rt = ctx->runtime;

    while (true) {
        pthread_mutex_lock(&r->mutex);
        bool active   = r->active;
        bool complete = r->complete;
        bool pending  = r->approval_pending;
        hive_status_t result = r->result;
        pthread_mutex_unlock(&r->mutex);

        erase();
        attron(A_BOLD);
        mvprintw(1, 2, active ? "RUN IN PROGRESS" : "RUN COMPLETE");
        attroff(A_BOLD);

        mvprintw(3, 2, "Workspace:    %s", safe_text(rt->session.workspace_root));
        mvprintw(4, 2, "Prompt:       %s", safe_text(rt->session.user_prompt));
        mvprintw(6, 2, "Score:        %u", rt->session.last_score.overall);
        mvprintw(7, 2, "Demand depth: %u",
                 rt->dynamics.demand_buffer_depth);
        mvprintw(8, 2, "Total agents: %u", rt->dynamics.stats.total_agents);

        if (complete) {
            mvprintw(10, 2, "Result:  %s",
                     result == HIVE_STATUS_OK ? "OK" : "error");
            mvprintw(11, 2, "Output:  %s",
                     safe_text(rt->session.final_output));
        }

        if (pending) {
            attron(A_BOLD | A_REVERSE);
            mvprintw(LINES - 4, 2, "  [!] TOOL APPROVAL REQUIRED  ");
            attroff(A_BOLD | A_REVERSE);
            mvprintw(LINES - 3, 2, "[A] Approve/deny  [Q] Back to menu");
        } else if (active) {
            mvprintw(LINES - 3, 2,
                     "[Q] Back to menu (run continues in background)");
        } else {
            mvprintw(LINES - 3, 2, "[Q] Back to menu");
        }
        refresh();

        int key = getch();
        if (key == ERR) {
            if (complete && !active) break;
            continue;
        }
        if (key == 'q' || key == 'Q' || key == 27) break;
        if ((key == 'a' || key == 'A') && pending)
            handle_pending_approval(ctx);
    }
}

/* ================================================================
 * Main TUI loop
 * ================================================================ */

hive_status_t hive_tui_run(hive_runtime_t *runtime)
{
    if (runtime == NULL)
        return HIVE_STATUS_INVALID_ARGUMENT;

    hive_tui_context_t context = {.runtime = runtime, .run = {0}};
    pthread_mutex_init(&context.run.mutex, NULL);
    pthread_cond_init(&context.run.approval_cond, NULL);

    runtime->tools.auto_approve       = false;
    runtime->tools.approval_fn        = approval_callback;
    runtime->tools.approval_user_data = &context;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    (void)curs_set(0);
    halfdelay(2);  /* 200 ms timeout — allows polling run state */

    while (true) {
        pthread_mutex_lock(&context.run.mutex);
        bool run_active   = context.run.active;
        bool run_complete = context.run.complete;
        bool run_pending  = context.run.approval_pending;
        pthread_mutex_unlock(&context.run.mutex);

        erase();

        if (run_active) {
            attron(A_BOLD);
            mvprintw(0, 2, "[ RUN ACTIVE ]");
            attroff(A_BOLD);
        }
        if (run_pending) {
            attron(A_BOLD | A_REVERSE);
            mvprintw(0, 20, " [!] APPROVAL REQUIRED — press A ");
            attroff(A_BOLD | A_REVERSE);
        }

        mvprintw(2, (COLS - 20) / 2, "HIVE RUNTIME MENU");
        if (run_active)
            mvprintw(4, 2, "1) View run status");
        else if (run_complete)
            mvprintw(4, 2, "1) View last run / Run again");
        else
            mvprintw(4, 2, "1) Run execution");
        mvprintw(5,  2, "2) Dashboard");
        mvprintw(6,  2, "3) Backlog");
        mvprintw(7,  2, "4) Status");
        mvprintw(8,  2, "5) Plan Mode");
        mvprintw(9,  2, "6) Settings");
        mvprintw(10, 2, "7) Quit");

        if (run_complete && !run_active)
            mvprintw(12, 2, "Last run score: %u",
                     runtime->session.last_score.overall);

        refresh();

        int key = getch();
        if (key == ERR) continue;

        /* Global approval shortcut */
        if (run_pending && (key == 'a' || key == 'A')) {
            handle_pending_approval(&context);
            continue;
        }

        if (key == '1') {
            if (run_active || run_pending) {
                show_run_status_page(&context);
            } else if (run_complete) {
                show_run_status_page(&context);
            } else {
                pthread_mutex_lock(&context.run.mutex);
                context.run.active   = true;
                context.run.complete = false;
                context.run.started  = true;
                pthread_mutex_unlock(&context.run.mutex);
                pthread_create(&context.run.thread, NULL,
                               bg_run_thread, &context);
            }
        } else if (key == '2') {
            hive_tui_page_dashboard(runtime);
        } else if (key == '3') {
            hive_tui_page_backlog(runtime);
        } else if (key == '4') {
            hive_tui_page_status(runtime);
        } else if (key == '5') {
            hive_tui_page_queen(runtime);
        } else if (key == '6') {
            hive_tui_page_settings(runtime, run_active);
        } else if (key == '7' || key == 27) {
            if (run_active) {
                erase();
                mvprintw(LINES / 2,     (COLS - 30) / 2,
                         "A run is still active.");
                mvprintw(LINES / 2 + 1, (COLS - 42) / 2,
                         "Quit anyway? Run will be abandoned. [Y/N]");
                refresh();
                int confirm = getch();
                if (confirm == ERR || (confirm != 'y' && confirm != 'Y'))
                    continue;
            }
            break;
        }
    }

    /* Clean up run thread if started */
    if (context.run.started) {
        /* Unblock any waiting approval so the thread can exit */
        pthread_mutex_lock(&context.run.mutex);
        if (context.run.approval_pending) {
            context.run.approval_result  = false;
            context.run.approval_pending = false;
            pthread_cond_signal(&context.run.approval_cond);
        }
        pthread_mutex_unlock(&context.run.mutex);
        pthread_join(context.run.thread, NULL);
    }

    endwin();
    pthread_cond_destroy(&context.run.approval_cond);
    pthread_mutex_destroy(&context.run.mutex);
    return HIVE_STATUS_OK;
}

#else

hive_status_t hive_tui_run(hive_runtime_t *runtime)
{
    (void)runtime;
    return HIVE_STATUS_UNAVAILABLE;
}

#endif
