#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "cli/cli.h"

#include "api/server.h"
#include "core/runtime.h"
#include "tui/tui.h"

#if CHARNESS_HAVE_ARGP
#include <argp.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct charness_cli_state {
    charness_runtime_options_t runtime_options;
    bool prompt_overridden;
    bool show_help;
    bool show_version;
} charness_cli_state_t;

static const char *default_prompt(void)
{
    return "Create a safe, production-grade C23 agent harness repository skeleton for hive.";
}

static void set_defaults(charness_cli_state_t *state)
{
    memset(state, 0, sizeof(*state));
    state->runtime_options.workspace_root = ".";
    state->runtime_options.user_prompt = default_prompt();
    state->runtime_options.rules_path = NULL;
    state->runtime_options.api_bind_address = "127.0.0.1";
    state->runtime_options.api_port = 8080U;
    state->runtime_options.max_iterations = 3U;
    state->runtime_options.auto_approve = false;
    state->runtime_options.enable_tui = false;
    state->runtime_options.enable_api = false;
    state->runtime_options.use_mock_inference = true;
    state->runtime_options.enable_syslog = true;
    state->runtime_options.approval_fn = NULL;
    state->runtime_options.approval_user_data = NULL;
}

static bool parse_unsigned(const char *text, unsigned *value_out)
{
    if (text == NULL || value_out == NULL) {
        return false;
    }

    errno = 0;
    char *end = NULL;
    const unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > 65535UL) {
        return false;
    }

    *value_out = (unsigned)parsed;
    return true;
}

static void print_help(FILE *stream, const char *program_name)
{
    fprintf(stream,
            "Usage: %s [OPTIONS] [PROMPT]\n\n"
            "Options:\n"
            "  --workspace DIR     Set the workspace root\n"
            "  --prompt TEXT       Set the user prompt\n"
            "  --rules FILE        Load extra project rules from a file\n"
            "  --iterations N      Set the maximum refinement loops\n"
            "  --yes               Auto-approve tool execution in headless mode\n"
            "  --tui               Launch the ncurses TUI\n"
            "  --api               Launch the optional API server\n"
            "  --api-bind ADDR     Bind address for the API server\n"
            "  --api-port PORT     Bind port for the API server\n"
            "  --no-syslog         Disable syslog duplication\n"
            "  --help              Show this help text\n"
            "  --version           Show version information\n",
            program_name != NULL ? program_name : "hive");
}

static void print_version(FILE *stream)
{
    fputs("hive 0.1.0\n", stream);
}

#if CHARNESS_HAVE_ARGP
static const struct argp_option argp_options[] = {
    {"workspace", 'w', "DIR", 0, "Set the workspace root", 0},
    {"prompt", 'p', "TEXT", 0, "Set the user prompt", 0},
    {"rules", 'r', "FILE", 0, "Load extra project rules from a file", 0},
    {"iterations", 'i', "N", 0, "Set the maximum refinement loops", 0},
    {"yes", 'y', 0, 0, "Auto-approve tool execution in headless mode", 0},
    {"tui", 't', 0, 0, "Launch the ncurses TUI", 0},
    {"api", 'a', 0, 0, "Launch the optional API server", 0},
    {"api-bind", 1000, "ADDR", 0, "Bind address for the API server", 0},
    {"api-port", 1001, "PORT", 0, "Bind port for the API server", 0},
    {"no-syslog", 1002, 0, 0, "Disable syslog duplication", 0},
    {0, 0, 0, 0, 0, 0},
};

static error_t parse_argp_option(int key, char *arg, struct argp_state *state)
{
    charness_cli_state_t *cli_state = state->input;

    switch (key) {
    case 'w':
        cli_state->runtime_options.workspace_root = arg;
        break;
    case 'p':
        cli_state->runtime_options.user_prompt = arg;
        cli_state->prompt_overridden = true;
        break;
    case 'r':
        cli_state->runtime_options.rules_path = arg;
        break;
    case 'i':
        if (!parse_unsigned(arg, &cli_state->runtime_options.max_iterations)) {
            return ARGP_ERR_UNKNOWN;
        }
        break;
    case 'y':
        cli_state->runtime_options.auto_approve = true;
        break;
    case 't':
        cli_state->runtime_options.enable_tui = true;
        break;
    case 'a':
        cli_state->runtime_options.enable_api = true;
        break;
    case 1000:
        cli_state->runtime_options.api_bind_address = arg;
        break;
    case 1001:
        if (!parse_unsigned(arg, &cli_state->runtime_options.api_port)) {
            return ARGP_ERR_UNKNOWN;
        }
        break;
    case 1002:
        cli_state->runtime_options.enable_syslog = false;
        break;
    case ARGP_KEY_ARG:
        if (!cli_state->prompt_overridden) {
            cli_state->runtime_options.user_prompt = arg;
            cli_state->prompt_overridden = true;
        }
        break;
    case ARGP_KEY_END:
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

static const struct argp argp_parser = {
    .options = argp_options,
    .parser = parse_argp_option,
    .args_doc = "[PROMPT]",
    .doc = "hive - C Agent Harness",
};
#endif

static charness_status_t run_application(const char *program_name, charness_cli_state_t *state)
{
    if (state->runtime_options.enable_tui && state->runtime_options.enable_api) {
        fprintf(stderr, "hive: --tui and --api are mutually exclusive\n");
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    charness_runtime_t runtime;
    charness_status_t status = charness_runtime_init(&runtime, &state->runtime_options, program_name);
    if (status != CHARNESS_STATUS_OK) {
        fprintf(stderr, "hive: runtime init failed: %s\n", charness_status_to_string(status));
        return status;
    }

    if (state->runtime_options.enable_api) {
        status = charness_api_server_run(&runtime);
    } else if (state->runtime_options.enable_tui) {
        status = charness_tui_run(&runtime);
    } else {
        status = charness_runtime_run(&runtime);
    }

    charness_runtime_deinit(&runtime);
    return status;
}

charness_status_t charness_cli_run(int argc, char **argv)
{
    const char *program_name = (argc > 0 && argv != NULL && argv[0] != NULL) ? argv[0] : "hive";
    charness_cli_state_t state;
    set_defaults(&state);

#if CHARNESS_HAVE_ARGP
    argp_parse(&argp_parser, argc, argv, 0, 0, &state);
#else
    for (int index = 1; index < argc; ++index) {
        const char *argument = argv[index];
        if (strcmp(argument, "--help") == 0) {
            state.show_help = true;
        } else if (strcmp(argument, "--version") == 0) {
            state.show_version = true;
        } else if (strcmp(argument, "--yes") == 0) {
            state.runtime_options.auto_approve = true;
        } else if (strcmp(argument, "--tui") == 0) {
            state.runtime_options.enable_tui = true;
        } else if (strcmp(argument, "--api") == 0) {
            state.runtime_options.enable_api = true;
        } else if (strcmp(argument, "--no-syslog") == 0) {
            state.runtime_options.enable_syslog = false;
        } else if (strcmp(argument, "--workspace") == 0 && index + 1 < argc) {
            state.runtime_options.workspace_root = argv[++index];
        } else if (strcmp(argument, "--prompt") == 0 && index + 1 < argc) {
            state.runtime_options.user_prompt = argv[++index];
            state.prompt_overridden = true;
        } else if (strcmp(argument, "--rules") == 0 && index + 1 < argc) {
            state.runtime_options.rules_path = argv[++index];
        } else if (strcmp(argument, "--iterations") == 0 && index + 1 < argc) {
            if (!parse_unsigned(argv[++index], &state.runtime_options.max_iterations)) {
                fprintf(stderr, "hive: invalid iteration count\n");
                return CHARNESS_STATUS_INVALID_ARGUMENT;
            }
        } else if (strcmp(argument, "--api-bind") == 0 && index + 1 < argc) {
            state.runtime_options.api_bind_address = argv[++index];
        } else if (strcmp(argument, "--api-port") == 0 && index + 1 < argc) {
            if (!parse_unsigned(argv[++index], &state.runtime_options.api_port)) {
                fprintf(stderr, "hive: invalid API port\n");
                return CHARNESS_STATUS_INVALID_ARGUMENT;
            }
        } else if (argument[0] == '-' && argument[1] == '-') {
            fprintf(stderr, "hive: unknown option: %s\n", argument);
            return CHARNESS_STATUS_INVALID_ARGUMENT;
        } else if (!state.prompt_overridden) {
            state.runtime_options.user_prompt = argument;
            state.prompt_overridden = true;
        }
    }
#endif

    if (state.show_help) {
        print_help(stdout, program_name);
        return CHARNESS_STATUS_OK;
    }

    if (state.show_version) {
        print_version(stdout);
        return CHARNESS_STATUS_OK;
    }

    return run_application(program_name, &state);
}
