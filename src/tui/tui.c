#include "tui/tui.h"

#include "core/runtime.h"
#include "logging/logger.h"

#if CHARNESS_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif

#include <stdbool.h>
#include <stdio.h>

typedef struct charness_tui_context {
    charness_runtime_t *runtime;
} charness_tui_context_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static bool approval_callback(void *user_data, const char *tool_name, const char *details)
{
    charness_tui_context_t *context = user_data;
    (void)context;

    erase();
    mvprintw(0, 0, "charness tool approval required");
    mvprintw(2, 0, "Tool: %s", safe_text(tool_name));
    mvprintw(4, 0, "Request: %s", safe_text(details));
    mvprintw(6, 0, "Approve execution? [y/N]");
    refresh();

    while (true) {
        const int key = getch();
        if (key == 'y' || key == 'Y') {
            if (context != NULL && context->runtime != NULL && context->runtime->logger.initialized) {
                charness_logger_log(&context->runtime->logger, CHARNESS_LOG_INFO, "tui", "approval", "approved tool request");
            }
            return true;
        }
        if (key == 'n' || key == 'N' || key == 27) {
            if (context != NULL && context->runtime != NULL && context->runtime->logger.initialized) {
                charness_logger_log(&context->runtime->logger, CHARNESS_LOG_WARN, "tui", "approval", "denied tool request");
            }
            return false;
        }
    }
}

static void show_final_summary(charness_runtime_t *runtime)
{
    erase();
    mvprintw(0, 0, "charness run complete");
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

charness_status_t charness_tui_run(charness_runtime_t *runtime)
{
    if (runtime == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    runtime->tools.auto_approve = false;
    charness_tui_context_t context = {
        .runtime = runtime,
    };
    runtime->tools.approval_fn = approval_callback;
    runtime->tools.approval_user_data = &context;

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    (void)curs_set(0);

    erase();
    mvprintw(0, 0, "charness TUI");
    mvprintw(2, 0, "Workspace: %s", safe_text(runtime->session.workspace_root));
    mvprintw(3, 0, "Prompt: %s", safe_text(runtime->session.user_prompt));
    mvprintw(5, 0, "Tool execution is gated interactively.");
    mvprintw(6, 0, "The runtime will now execute the hierarchical agent loop.");
    refresh();

    const charness_status_t status = charness_runtime_run(runtime);
    show_final_summary(runtime);

    endwin();
    return status;
}

#else

charness_status_t charness_tui_run(charness_runtime_t *runtime)
{
    (void)runtime;
    return CHARNESS_STATUS_UNAVAILABLE;
}

#endif
