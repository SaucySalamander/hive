#include "raygui/raygui.h"

#include "core/runtime.h"
#include "logging/logger.h"

#if HIVE_HAVE_RAYGUI

#include <raylib.h>
#include <raygui.h>

#include <stdbool.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

typedef struct hive_raygui_context {
    hive_runtime_t *runtime;
} hive_raygui_context_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

/* VSCode-like color palette */
static Color vg_bg = {30, 30, 30, 255};
static Color vg_sidebar = {37, 37, 38, 255};
static Color vg_panel = {45, 45, 48, 255};
static Color vg_text = {212, 212, 212, 255};
static Color vg_muted = {133, 133, 133, 255};
static Color vg_accent = {0, 122, 204, 255};

/* Font used for rendering; may be loaded from system or assets. */
static Font hive_font;
static bool hive_font_loaded = false;

static void draw_vscode_shell(hive_runtime_t *runtime)
{
    const int w = GetScreenWidth();
    const int h = GetScreenHeight();

    // Background
    DrawRectangle(0, 0, w, h, vg_bg);

    // Sidebar
    const int sidebar_w = 260;
    DrawRectangle(0, 0, sidebar_w, h, vg_sidebar);
    DrawTextEx(hive_font, "Explorer", (Vector2){20, 12}, 16, 0, vg_text);

    // Top tab bar
    DrawRectangle(sidebar_w, 0, w - sidebar_w, 36, vg_panel);
    DrawRectangle(sidebar_w + 8, 6, 160, 24, vg_panel);
    DrawTextEx(hive_font, "main.c", (Vector2){sidebar_w + 16, 8}, 16, 0, vg_text);

    // Editor area
    DrawRectangle(sidebar_w, 36, w - sidebar_w, h - 36 - 28, (Color){255,255,255,8});
    DrawTextEx(hive_font, runtime->session.user_prompt, (Vector2){sidebar_w + 12, 48}, 14, 0, vg_muted);

    // Status bar
    DrawRectangle(0, h - 28, w, 28, vg_panel);
    DrawTextEx(hive_font, "Ready", (Vector2){12, h - 22}, 12, 0, vg_muted);
}

static bool approval_callback(void *user_data, const char *tool_name, const char *details)
{
    hive_raygui_context_t *context = user_data;
    hive_runtime_t *runtime = context != NULL ? context->runtime : NULL;

    // Modal loop: dim background and show prompt
    while (!WindowShouldClose()) {
        BeginDrawing();
        // Dim background
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0,0,0,120});

        // Modal box
        const int mw = 640;
        const int mh = 160;
        const int mx = (GetScreenWidth() - mw) / 2;
        const int my = (GetScreenHeight() - mh) / 2;
        DrawRectangle(mx, my, mw, mh, vg_panel);
        DrawRectangleLines(mx, my, mw, mh, vg_accent);

        DrawTextEx(hive_font, "hive: tool approval required", (Vector2){mx + 16, my + 12}, 20, 0, vg_text);
        DrawTextEx(hive_font, TextFormat("Tool: %s", safe_text(tool_name)), (Vector2){mx + 16, my + 48}, 16, 0, vg_text);
        DrawTextEx(hive_font, TextFormat("Request: %s", safe_text(details)), (Vector2){mx + 16, my + 76}, 14, 0, vg_muted);
        DrawTextEx(hive_font, "Press Y to approve, N to deny", (Vector2){mx + 16, my + 108}, 14, 0, vg_text);
        EndDrawing();

        if (IsKeyPressed(KEY_Y)) {
            if (runtime != NULL && runtime->logger.initialized) {
                hive_logger_log(&runtime->logger, HIVE_LOG_INFO, "raygui", "approval", "approved tool request");
            }
            return true;
        }
        if (IsKeyPressed(KEY_N) || IsKeyPressed(KEY_ESCAPE)) {
            if (runtime != NULL && runtime->logger.initialized) {
                hive_logger_log(&runtime->logger, HIVE_LOG_WARN, "raygui", "approval", "denied tool request");
            }
            return false;
        }
    }

    return false;
}

static void show_final_summary(hive_runtime_t *runtime)
{
    while (!WindowShouldClose()) {
        BeginDrawing();
        draw_vscode_shell(runtime);
        DrawTextEx(hive_font, "hive run complete", (Vector2){280, 44}, 22, 0, vg_text);
        DrawTextEx(hive_font, TextFormat("Overall score: %u", runtime->session.last_score.overall), (Vector2){280, 84}, 16, 0, vg_text);
        DrawTextEx(hive_font, "Press any key or close window to exit.", (Vector2){280, 140}, 14, 0, vg_muted);
        EndDrawing();
        // Treat any key or mouse click as continue
        bool cont = false;
        for (int k = 0; k < 512; ++k) {
            if (IsKeyPressed(k)) { cont = true; break; }
        }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) cont = true;
        if (cont) break;
    }
}

/* Font management: try to load a preferred monospace font for better appearance. */

static void try_load_preferred_font(void)
{
    if (hive_font_loaded) return;

    const char *candidates[] = {
        "assets/fonts/CascadiaCode.ttf",
        "assets/fonts/RobotoMono-Regular.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuMono-R.ttf",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; ++i) {
        if (FileExists(candidates[i])) {
            hive_font = LoadFontEx(candidates[i], 16, (int *)0, 0);
            hive_font_loaded = true;
            return;
        }
    }

    // If no exact candidate matched, scan the assets/fonts directory for any TTF/OTF
    DIR *d = opendir("assets/fonts");
    if (d != NULL) {
        struct dirent *ent;
        char path[PATH_MAX];
        while ((ent = readdir(d)) != NULL) {
            const char *name = ent->d_name;
            size_t len = strlen(name);
            if (len > 4) {
                const char *ext = name + len - 4;
                if (strcasecmp(ext, ".ttf") == 0 || strcasecmp(ext, ".otf") == 0) {
                    snprintf(path, sizeof(path), "assets/fonts/%s", name);
                    hive_font = LoadFontEx(path, 16, (int *)0, 0);
                    hive_font_loaded = true;
                    closedir(d);
                    return;
                }
            }
        }
        closedir(d);
    }

    hive_font = GetFontDefault();
}

hive_status_t hive_raygui_run(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    runtime->tools.auto_approve = false;
    hive_raygui_context_t context = {
        .runtime = runtime,
    };
    runtime->tools.approval_fn = approval_callback;
    runtime->tools.approval_user_data = &context;

    // Minimal Raylib + Raygui window initialization.
    const int width = 1024;
    const int height = 768;
    InitWindow(width, height, "hive - RayGUI (VSCode style)");
    SetTargetFPS(60);

    try_load_preferred_font();

    // Styled landing screen before running the runtime loop.
    while (!WindowShouldClose()) {
        BeginDrawing();
        draw_vscode_shell(runtime);
        DrawTextEx(hive_font, "Press Enter to start the runtime", (Vector2){280, 120}, 16, 0, vg_muted);
        EndDrawing();

        if (IsKeyPressed(KEY_ENTER)) break;
    }

    const hive_status_t status = hive_runtime_run(runtime);
    show_final_summary(runtime);

    if (hive_font_loaded) {
        UnloadFont(hive_font);
        hive_font_loaded = false;
    }

    CloseWindow();
    return status;
}

#else

hive_status_t hive_raygui_run(hive_runtime_t *runtime)
{
    if (runtime != NULL) {
        if (runtime->logger.initialized) {
            hive_logger_log(&runtime->logger, HIVE_LOG_ERROR, "raygui", "unavailable", "RayGUI support not compiled into this build");
        } else {
            fprintf(stderr, "hive: RayGUI support not compiled into this build\n");
        }
    } else {
        fprintf(stderr, "hive: RayGUI support not compiled into this build\n");
    }
    return HIVE_STATUS_UNAVAILABLE;
}

#endif
