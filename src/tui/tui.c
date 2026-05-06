#include "tui/tui.h"
#include "tui/pages/queen.h"
#include "tui/pages/dashboard.h"
#include "tui/pages/backlog.h"
#include "tui/pages/status.h"

#include "core/runtime.h"
#include "common/logging/logger.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif

#include <stdbool.h>
#include <stdio.h>

typedef struct hive_tui_context {
    hive_runtime_t *runtime;
} hive_tui_context_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static bool approval_callback(void *user_data, const char *tool_name, const char *details)
{
    hive_tui_context_t *context = user_data;
    (void)context;

    erase();
    mvprintw(0, 0, "hive tool approval required");
    mvprintw(2, 0, "Tool: %s", safe_text(tool_name));
    mvprintw(4, 0, "Request: %s", safe_text(details));
    mvprintw(6, 0, "Approve execution? [y/N]");
    refresh();

    while (true) {
        const int key = getch();
        if (key == 'y' || key == 'Y') {
            if (context != NULL && context->runtime != NULL && context->runtime->logger.initialized) {
                hive_logger_log(&context->runtime->logger, HIVE_LOG_INFO, "tui", "approval", "approved tool request");
            }
            return true;
        }
        if (key == 'n' || key == 'N' || key == 27) {
            if (context != NULL && context->runtime != NULL && context->runtime->logger.initialized) {
                hive_logger_log(&context->runtime->logger, HIVE_LOG_WARN, "tui", "approval", "denied tool request");
            }
            return false;
        }
    }
}

static void show_final_summary(hive_runtime_t *runtime)
{
    erase();
    mvprintw(0, 0, "hive run complete");
    mvprintw(2, 0, "Overall score: %u", runtime->session.last_score.overall);
    mvprintw(3, 0, "Correctness:   %u", runtime->session.last_score.correctness);
    mvprintw(4, 0, "Security:      %u", runtime->session.last_score.security);
    mvprintw(5, 0, "Style:         %u", runtime->session.last_score.style);
    mvprintw(6, 0, "Test coverage: %u", runtime->session.last_score.test_coverage);
    mvprintw(8, 0, "Final output:");
    mvprintw(9, 0, "%s", safe_text(runtime->session.final_output));
    mvprintw(LINES - 2, 0, "Press any key to exit.");
    refresh();
    (void)getch();
}

hive_status_t hive_tui_run(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    runtime->tools.auto_approve = false;
    hive_tui_context_t context = {
        .runtime = runtime,
    };
    runtime->tools.approval_fn = approval_callback;
    runtime->tools.approval_user_data = &context;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    (void)curs_set(0);

    /* Show main menu */
    while (true) {
        erase();
        mvprintw(2, (COLS - 20) / 2, "HIVE RUNTIME MENU");
        mvprintw(4, 2, "1) Run regular execution");
        mvprintw(5, 2, "2) Dashboard (view workers and demand)");
        mvprintw(6, 2, "3) Backlog (view queue and models)");
        mvprintw(7, 2, "4) Status (view colony health)");
        mvprintw(8, 2, "5) Plan Mode (create and approve plans)");
        mvprintw(9, 2, "6) Quit");
        mvprintw(11, 2, "Select an option: ");
        refresh();

        int key = getch();
        
        if (key == '1') {
            /* Restore terminal before running — prevents stdout/stderr fighting ncurses */
            endwin();
            (void)hive_runtime_run(runtime);
            /* Re-init ncurses for the summary screen */
            initscr();
            cbreak();
            noecho();
            keypad(stdscr, TRUE);
            (void)curs_set(0);
            show_final_summary(runtime);
            break;
        } else if (key == '2') {
            /* Dashboard */
            hive_tui_page_dashboard(runtime);
        } else if (key == '3') {
            /* Backlog */
            hive_tui_page_backlog(runtime);
        } else if (key == '4') {
            /* Status */
            hive_tui_page_status(runtime);
        } else if (key == '5') {
            /* Plan mode */
            hive_tui_page_queen(runtime);
        } else if (key == '6' || key == 27) {
            /* Quit */
            break;
        }
    }

    endwin();
    return HIVE_STATUS_OK;
}

#else

hive_status_t hive_tui_run(hive_runtime_t *runtime)
{
    (void)runtime;
    return HIVE_STATUS_UNAVAILABLE;
}

#endif
