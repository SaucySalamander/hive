#include "core/runtime.h"

#include "common/strings.h"
#include "core/orchestrator.h"
#include "tools/registry.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static const char *default_workspace_root(void)
{
    return ".";
}

static const char *default_prompt(void)
{
    return "Create a production-ready C23 repository skeleton for hive.";
}

static const char *default_api_bind_address(void)
{
    return "127.0.0.1";
}

static const char *default_project_rules(void)
{
    return
    "# hive default rules\n"
        "- Prefer correctness and leak-free code.\n"
        "- Keep tool execution behind an approval gate.\n"
        "- Preserve portable C23 and POSIX compatibility.\n"
        "- Use the evaluator and reflexion layers before finalizing results.\n";
}

static bool file_exists(const char *path)
{
    struct stat st;
    return path != NULL && stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static char *read_text_file(const char *path)
{
    if (path == NULL) {
        return NULL;
    }

    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return NULL;
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }

    long size = ftell(file);
    if (size < 0L) {
        fclose(file);
        return NULL;
    }

    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }

    char *text = malloc((size_t)size + 1U);
    if (text == NULL) {
        fclose(file);
        return NULL;
    }

    const size_t bytes_read = fread(text, 1U, (size_t)size, file);
    fclose(file);
    if (bytes_read != (size_t)size) {
        free(text);
        return NULL;
    }

    text[bytes_read] = '\0';
    return text;
}

static char *load_project_rules(const char *requested_path)
{
    if (requested_path != NULL && requested_path[0] != '\0') {
        char *text = read_text_file(requested_path);
        if (text != NULL) {
            return text;
        }
    }

    if (file_exists("config/project.rules.example")) {
        char *text = read_text_file("config/project.rules.example");
        if (text != NULL) {
            return text;
        }
    }

    return hive_string_dup(default_project_rules());
}

hive_status_t hive_runtime_init(hive_runtime_t *runtime,
                                        const hive_runtime_options_t *options,
                                        const char *program_name)
{
    if (runtime == NULL || options == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    memset(runtime, 0, sizeof(*runtime));
    runtime->options = *options;

    if (runtime->options.workspace_root == NULL || runtime->options.workspace_root[0] == '\0') {
        runtime->options.workspace_root = default_workspace_root();
    }
    if (runtime->options.user_prompt == NULL || runtime->options.user_prompt[0] == '\0') {
        runtime->options.user_prompt = default_prompt();
    }
    if (runtime->options.api_bind_address == NULL || runtime->options.api_bind_address[0] == '\0') {
        runtime->options.api_bind_address = default_api_bind_address();
    }
    if (runtime->options.api_port == 0U) {
        runtime->options.api_port = 8080U;
    }
    if (runtime->options.max_iterations == 0U) {
        runtime->options.max_iterations = 3U;
    }

    hive_status_t status = hive_logger_init(&runtime->logger,
                                                    program_name,
                                                    runtime->options.enable_syslog,
                                                    HIVE_LOG_INFO);
    if (status != HIVE_STATUS_OK) {
        return status;
    }

    status = hive_inference_adapter_init_mock(&runtime->adapter, &runtime->logger);
    if (status != HIVE_STATUS_OK) {
        hive_logger_deinit(&runtime->logger);
        return status;
    }

    char *rules_text = load_project_rules(runtime->options.rules_path);
    if (rules_text == NULL) {
        hive_inference_adapter_deinit(&runtime->adapter);
        hive_logger_deinit(&runtime->logger);
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    status = hive_session_init(&runtime->session,
                                   runtime->options.workspace_root,
                                   runtime->options.user_prompt,
                                   rules_text);
    free(rules_text);
    if (status != HIVE_STATUS_OK) {
        hive_inference_adapter_deinit(&runtime->adapter);
        hive_logger_deinit(&runtime->logger);
        return status;
    }

    status = hive_tool_registry_init(&runtime->tools,
                                         runtime->session.workspace_root,
                                         &runtime->logger,
                                         runtime->options.approval_fn,
                                         runtime->options.approval_user_data,
                                         runtime->options.auto_approve);
    if (status != HIVE_STATUS_OK) {
        hive_session_deinit(&runtime->session);
        hive_inference_adapter_deinit(&runtime->adapter);
        hive_logger_deinit(&runtime->logger);
        return status;
    }

    status = hive_state_machine_init(&runtime->machine, runtime->options.max_iterations);
    if (status != HIVE_STATUS_OK) {
        hive_tool_registry_deinit(&runtime->tools);
        hive_session_deinit(&runtime->session);
        hive_inference_adapter_deinit(&runtime->adapter);
        hive_logger_deinit(&runtime->logger);
        return status;
    }

    hive_logger_logf(&runtime->logger,
                         HIVE_LOG_INFO,
                         "runtime",
                         "init",
                         "workspace=%s iterations=%u",
                         runtime->session.workspace_root,
                         runtime->options.max_iterations);
    return HIVE_STATUS_OK;
}

void hive_runtime_deinit(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return;
    }

    hive_tool_registry_deinit(&runtime->tools);
    hive_session_deinit(&runtime->session);
    hive_inference_adapter_deinit(&runtime->adapter);
    hive_logger_deinit(&runtime->logger);
    memset(runtime, 0, sizeof(*runtime));
}

hive_status_t hive_runtime_run(hive_runtime_t *runtime)
{
    if (runtime == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    return hive_state_machine_run(&runtime->machine, runtime);
}
