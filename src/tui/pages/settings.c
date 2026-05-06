#include "tui/pages/settings.h"
#include "core/runtime.h"
#include "core/inference/adapter.h"

#if HIVE_HAVE_NCURSES
#if __has_include(<ncurses.h>)
#include <ncurses.h>
#elif __has_include(<curses.h>)
#include <curses.h>
#endif

#include <stdlib.h>
#include <string.h>

/* Backends available for selection */
static const char *const BACKENDS[]  = { "mock", "copilotcli" };
static const int          N_BACKENDS = 2;

#define CONFIG_JSON_MAX 512

typedef struct {
    int   backend_idx;
    char  config_json[CONFIG_JSON_MAX];
    int   config_cursor;
    int   active_field; /* 0 = backend selector, 1 = config json */
    char  status_msg[256];
    bool  status_ok;
} settings_state_t;

static void draw_settings(const settings_state_t *s, bool run_active)
{
    erase();
    attron(A_BOLD);
    mvprintw(0, 2, "Settings — Inference Backend");
    attroff(A_BOLD);

    mvprintw(2, 2, "[Tab] Next field   [Enter/Space] Edit   [A] Apply   [Q] Back");

    if (run_active) {
        attron(A_BOLD | A_REVERSE);
        mvprintw(3, 2, " Run in progress — Apply disabled ");
        attroff(A_BOLD | A_REVERSE);
    }

    /* Field 0: backend selector */
    int row_backend = 5;
    if (s->active_field == 0)
        attron(A_REVERSE);
    mvprintw(row_backend, 2, "Backend:  [<] %s [>]",
             BACKENDS[s->backend_idx]);
    if (s->active_field == 0)
        attroff(A_REVERSE);

    /* Field 1: config JSON */
    int row_config = 7;
    mvprintw(row_config,     2, "Config JSON:");
    if (s->active_field == 1)
        attron(A_REVERSE);

    int max_w = COLS - 4;
    int vis_start = (s->config_cursor >= max_w) ? s->config_cursor - max_w + 1 : 0;
    char vis_buf[CONFIG_JSON_MAX + 1];
    int  vis_len = (int)strlen(s->config_json) - vis_start;
    if (vis_len < 0) vis_len = 0;
    strncpy(vis_buf, s->config_json + vis_start, (size_t)vis_len);
    vis_buf[vis_len] = '\0';
    mvprintw(row_config + 1, 2, "%-*s", max_w, vis_buf);
    if (s->active_field == 1) {
        attroff(A_REVERSE);
        /* Cursor indicator */
        int cx = s->config_cursor - vis_start;
        mvchgat(row_config + 1, 2 + cx, 1, A_REVERSE, 0, NULL);
    }

    /* Current active adapter */
    mvprintw(LINES - 5, 2, "Active backend: %s",
             (s->active_field != -1 && s->backend_idx >= 0)
                 ? BACKENDS[s->backend_idx] : "unknown");

    /* Status line */
    if (s->status_msg[0] != '\0') {
        if (s->status_ok)
            attron(A_BOLD);
        mvprintw(LINES - 3, 2, "%s", s->status_msg);
        if (s->status_ok)
            attroff(A_BOLD);
    }
    mvprintw(LINES - 2, 2, "[Q] Back");
    refresh();
}

static void apply_backend(settings_state_t *s, hive_runtime_t *runtime)
{
    const char *name = BACKENDS[s->backend_idx];
    const char *cfg  = (s->config_json[0] != '\0') ? s->config_json : NULL;

    hive_inference_adapter_deinit(&runtime->adapter);
    hive_status_t st = hive_inference_adapter_init_named(
                           &runtime->adapter, name, cfg, &runtime->logger);

    if (st == HIVE_STATUS_OK) {
        setenv("HIVE_INFERENCE_BACKEND", name, 1);
        snprintf(s->status_msg, sizeof(s->status_msg),
                 "Applied: backend = %s", name);
        s->status_ok = true;
    } else {
        snprintf(s->status_msg, sizeof(s->status_msg),
                 "Error: failed to init backend '%s' (code %d)", name, (int)st);
        s->status_ok = false;
    }
}

static void edit_config_json(settings_state_t *s)
{
    int len = (int)strlen(s->config_json);
    int max_w = COLS - 4;

    while (true) {
        /* Redraw just the config line */
        int vis_start = (s->config_cursor >= max_w) ? s->config_cursor - max_w + 1 : 0;
        char vis_buf[CONFIG_JSON_MAX + 1];
        int  vis_len = len - vis_start;
        if (vis_len < 0) vis_len = 0;
        strncpy(vis_buf, s->config_json + vis_start, (size_t)vis_len);
        vis_buf[vis_len] = '\0';
        mvprintw(8, 2, "%-*s", max_w, vis_buf);
        int cx = s->config_cursor - vis_start;
        mvchgat(8, 2 + cx, 1, A_REVERSE, 0, NULL);
        refresh();

        int key = getch();
        if (key == ERR) continue;
        if (key == 27 || key == '\n' || key == KEY_ENTER) break;

        if (key == KEY_LEFT) {
            if (s->config_cursor > 0) s->config_cursor--;
        } else if (key == KEY_RIGHT) {
            if (s->config_cursor < len) s->config_cursor++;
        } else if (key == KEY_HOME) {
            s->config_cursor = 0;
        } else if (key == KEY_END) {
            s->config_cursor = len;
        } else if (key == KEY_BACKSPACE || key == 127) {
            if (s->config_cursor > 0 && len > 0) {
                memmove(s->config_json + s->config_cursor - 1,
                        s->config_json + s->config_cursor,
                        (size_t)(len - s->config_cursor + 1));
                s->config_cursor--;
                len--;
            }
        } else if (key == KEY_DC) {
            if (s->config_cursor < len) {
                memmove(s->config_json + s->config_cursor,
                        s->config_json + s->config_cursor + 1,
                        (size_t)(len - s->config_cursor));
                len--;
            }
        } else if (key > 0 && key < 128 && len < CONFIG_JSON_MAX - 1) {
            memmove(s->config_json + s->config_cursor + 1,
                    s->config_json + s->config_cursor,
                    (size_t)(len - s->config_cursor + 1));
            s->config_json[s->config_cursor] = (char)key;
            s->config_cursor++;
            len++;
        }
    }
}

hive_status_t hive_tui_page_settings(hive_runtime_t *runtime, bool run_active)
{
    settings_state_t s = {0};

    /* Pre-populate backend index from current adapter */
    if (!runtime->adapter.is_mock) {
        s.backend_idx = 1; /* copilotcli */
    }

    /* Pre-populate config from env */
    const char *env_cfg = getenv("HIVE_INFERENCE_CONFIG");
    if (env_cfg != NULL)
        strncpy(s.config_json, env_cfg, CONFIG_JSON_MAX - 1);

    while (true) {
        draw_settings(&s, run_active);

        int key = getch();
        if (key == ERR) continue;

        if (key == 'q' || key == 'Q' || key == 27) break;

        if (key == '\t') {
            s.active_field = (s.active_field + 1) % 2;
        } else if (key == 'a' || key == 'A') {
            if (!run_active)
                apply_backend(&s, runtime);
            else
                snprintf(s.status_msg, sizeof(s.status_msg),
                         "Cannot apply while run is active.");
        } else if (s.active_field == 0) {
            if (key == KEY_LEFT || key == KEY_RIGHT || key == ' ' || key == '\n') {
                s.backend_idx = (s.backend_idx + 1) % N_BACKENDS;
                s.status_msg[0] = '\0';
            }
        } else if (s.active_field == 1) {
            if (key == '\n' || key == KEY_ENTER) {
                edit_config_json(&s);
                s.status_msg[0] = '\0';
            }
        }
    }
    return HIVE_STATUS_OK;
}

#else

hive_status_t hive_tui_page_settings(hive_runtime_t *runtime, bool run_active)
{
    (void)runtime;
    (void)run_active;
    return HIVE_STATUS_UNAVAILABLE;
}

#endif
