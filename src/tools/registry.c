#include "tools/registry.h"

#include "common/strings.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct text_buffer {
    char *data;
    size_t length;
    size_t capacity;
} text_buffer_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static void buffer_init(text_buffer_t *buffer)
{
    if (buffer != NULL) {
        buffer->data = NULL;
        buffer->length = 0U;
        buffer->capacity = 0U;
    }
}

static void buffer_free(text_buffer_t *buffer)
{
    if (buffer != NULL) {
        free(buffer->data);
        buffer->data = NULL;
        buffer->length = 0U;
        buffer->capacity = 0U;
    }
}

static charness_status_t buffer_reserve(text_buffer_t *buffer, size_t required)
{
    if (buffer == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    if (required + 1U <= buffer->capacity) {
        return CHARNESS_STATUS_OK;
    }

    size_t next_capacity = buffer->capacity == 0U ? 256U : buffer->capacity;
    while (next_capacity < required + 1U) {
        next_capacity *= 2U;
    }

    char *next = realloc(buffer->data, next_capacity);
    if (next == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    buffer->data = next;
    buffer->capacity = next_capacity;
    return CHARNESS_STATUS_OK;
}

static charness_status_t buffer_append(text_buffer_t *buffer, const char *text)
{
    const char *safe = safe_text(text);
    const size_t length = strlen(safe);
    charness_status_t status = buffer_reserve(buffer, buffer->length + length);
    if (status != CHARNESS_STATUS_OK) {
        return status;
    }

    memcpy(buffer->data + buffer->length, safe, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
    return CHARNESS_STATUS_OK;
}

static charness_status_t buffer_appendf(text_buffer_t *buffer, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char *formatted = charness_string_vformat(format, args);
    va_end(args);

    if (formatted == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    const charness_status_t status = buffer_append(buffer, formatted);
    free(formatted);
    return status;
}

static char *buffer_steal(text_buffer_t *buffer)
{
    if (buffer == NULL) {
        return NULL;
    }

    char *data = buffer->data;
    buffer->data = NULL;
    buffer->length = 0U;
    buffer->capacity = 0U;
    return data;
}

static const char *workspace_or_default(const char *workspace_root)
{
    return workspace_root != NULL && workspace_root[0] != '\0' ? workspace_root : ".";
}

static bool path_is_absolute(const char *path)
{
    return path != NULL && path[0] == '/';
}

static char *resolve_path(const char *workspace_root, const char *path)
{
    if (path == NULL || path[0] == '\0') {
        return charness_string_dup(workspace_root);
    }

    if (path_is_absolute(path)) {
        return charness_string_dup(path);
    }

    return charness_string_format("%s/%s", workspace_or_default(workspace_root), path);
}

static char *read_text_file(const char *path)
{
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

static char *make_preview(const char *text)
{
    const char *safe = safe_text(text);
    size_t length = strlen(safe);
    if (length > 160U) {
        length = 160U;
    }

    char *preview = charness_string_ndup(safe, length);
    if (preview == NULL) {
        return NULL;
    }

    for (size_t index = 0U; preview[index] != '\0'; ++index) {
        if (preview[index] == '\n' || preview[index] == '\r' || preview[index] == '\t') {
            preview[index] = ' ';
        }
    }

    return preview;
}

static char *build_diff_preview(const char *path, const char *old_text, const char *new_text)
{
    char *old_preview = make_preview(old_text);
    char *new_preview = make_preview(new_text);
    if (old_preview == NULL || new_preview == NULL) {
        free(old_preview);
        free(new_preview);
        return NULL;
    }

    char *message = charness_string_format(
        "write_file diff preview for %s\n- %s\n+ %s\n",
        safe_text(path),
        old_preview,
        new_preview);
    free(old_preview);
    free(new_preview);
    return message;
}

static char *describe_request(const charness_tool_request_t *request)
{
    switch (request->kind) {
    case CHARNESS_TOOL_READ_FILE:
        return charness_string_format("read_file path=%s", safe_text(request->path));
    case CHARNESS_TOOL_WRITE_FILE:
        return charness_string_format("write_file path=%s", safe_text(request->path));
    case CHARNESS_TOOL_RUN_COMMAND:
        return charness_string_format("run_command command=%s", safe_text(request->command));
    case CHARNESS_TOOL_LIST_FILES:
        return charness_string_format("list_files path=%s", safe_text(request->path));
    case CHARNESS_TOOL_GREP:
        return charness_string_format("grep path=%s pattern=%s", safe_text(request->path), safe_text(request->pattern));
    case CHARNESS_TOOL_GIT_STATUS:
        return charness_string_format("git_status path=%s", safe_text(request->path));
    default:
        return charness_string_dup("unknown tool request");
    }
}

static char *read_directory_listing(const char *path)
{
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return NULL;
    }

    text_buffer_t buffer;
    buffer_init(&buffer);

    struct dirent *entry = NULL;
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (buffer_appendf(&buffer, "%s\n", entry->d_name) != CHARNESS_STATUS_OK) {
            buffer_free(&buffer);
            closedir(directory);
            return NULL;
        }
    }

    closedir(directory);
    return buffer_steal(&buffer);
}

static charness_status_t append_file_matches(text_buffer_t *buffer,
                                             const char *path,
                                             const char *pattern,
                                             unsigned *matches,
                                             unsigned max_matches);

static charness_status_t append_path_matches(text_buffer_t *buffer,
                                            const char *path,
                                            const char *pattern,
                                            unsigned *matches,
                                            unsigned max_matches)
{
    struct stat st;
    if (lstat(path, &st) != 0) {
        return CHARNESS_STATUS_OK;
    }

    if (S_ISDIR(st.st_mode)) {
        DIR *directory = opendir(path);
        if (directory == NULL) {
            return CHARNESS_STATUS_OK;
        }

        struct dirent *entry = NULL;
        while ((entry = readdir(directory)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char *child_path = charness_string_format("%s/%s", path, entry->d_name);
            if (child_path == NULL) {
                closedir(directory);
                return CHARNESS_STATUS_OUT_OF_MEMORY;
            }

            charness_status_t status = append_path_matches(buffer, child_path, pattern, matches, max_matches);
            free(child_path);
            if (status != CHARNESS_STATUS_OK) {
                closedir(directory);
                return status;
            }

            if (*matches >= max_matches) {
                break;
            }
        }

        closedir(directory);
        return CHARNESS_STATUS_OK;
    }

    if (S_ISREG(st.st_mode)) {
        return append_file_matches(buffer, path, pattern, matches, max_matches);
    }

    return CHARNESS_STATUS_OK;
}

static charness_status_t append_file_matches(text_buffer_t *buffer,
                                             const char *path,
                                             const char *pattern,
                                             unsigned *matches,
                                             unsigned max_matches)
{
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        return CHARNESS_STATUS_OK;
    }

    char line[4096];
    unsigned line_number = 1U;
    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, pattern) != NULL) {
            if (buffer_appendf(buffer, "%s:%u: %s", path, line_number, line) != CHARNESS_STATUS_OK) {
                fclose(file);
                return CHARNESS_STATUS_OUT_OF_MEMORY;
            }

            ++(*matches);
            if (*matches >= max_matches) {
                break;
            }
        }

        ++line_number;
    }

    fclose(file);
    return CHARNESS_STATUS_OK;
}

static char *recursive_grep(const char *root, const char *pattern)
{
    text_buffer_t buffer;
    buffer_init(&buffer);

    unsigned matches = 0U;
    charness_status_t status = append_path_matches(&buffer, root, pattern, &matches, 50U);
    if (status != CHARNESS_STATUS_OK) {
        buffer_free(&buffer);
        return NULL;
    }

    if (matches == 0U) {
        if (buffer_append(&buffer, "no matches\n") != CHARNESS_STATUS_OK) {
            buffer_free(&buffer);
            return NULL;
        }
    }

    return buffer_steal(&buffer);
}

static char *capture_command_output(const char *working_directory, char *const argv[])
{
    int pipefd[2];
    if (pipe(pipefd) != 0) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        (void)dup2(pipefd[1], STDOUT_FILENO);
        (void)dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        if (working_directory != NULL && chdir(working_directory) != 0) {
            _exit(127);
        }

        execvp(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);

    text_buffer_t buffer;
    buffer_init(&buffer);
    char chunk[4096];
    ssize_t bytes_read = 0;
    while ((bytes_read = read(pipefd[0], chunk, sizeof(chunk))) > 0) {
        if (buffer_reserve(&buffer, buffer.length + (size_t)bytes_read) != CHARNESS_STATUS_OK) {
            buffer_free(&buffer);
            close(pipefd[0]);
            (void)waitpid(pid, NULL, 0);
            return NULL;
        }

        memcpy(buffer.data + buffer.length, chunk, (size_t)bytes_read);
        buffer.length += (size_t)bytes_read;
        buffer.data[buffer.length] = '\0';
    }

    close(pipefd[0]);
    (void)waitpid(pid, NULL, 0);

    if (buffer.data == NULL) {
        return charness_string_dup("");
    }

    return buffer_steal(&buffer);
}

static char *git_status_output(const char *workspace_root)
{
    char *const argv[] = {
        "git",
        "-C",
        (char *)workspace_or_default(workspace_root),
        "status",
        "--short",
        "--branch",
        NULL,
    };

    return capture_command_output(workspace_or_default(workspace_root), argv);
}

const char *charness_tool_kind_to_string(charness_tool_kind_t kind)
{
    switch (kind) {
    case CHARNESS_TOOL_READ_FILE:
        return "read_file";
    case CHARNESS_TOOL_WRITE_FILE:
        return "write_file";
    case CHARNESS_TOOL_RUN_COMMAND:
        return "run_command";
    case CHARNESS_TOOL_LIST_FILES:
        return "list_files";
    case CHARNESS_TOOL_GREP:
        return "grep";
    case CHARNESS_TOOL_GIT_STATUS:
        return "git_status";
    default:
        return "unknown";
    }
}

charness_status_t charness_tool_registry_init(charness_tool_registry_t *registry,
                                              const char *workspace_root,
                                              charness_logger_t *logger,
                                              charness_tool_approval_fn approval_fn,
                                              void *approval_user_data,
                                              bool auto_approve)
{
    if (registry == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    registry->workspace_root = workspace_or_default(workspace_root);
    registry->logger = logger;
    registry->approval_fn = approval_fn;
    registry->approval_user_data = approval_user_data;
    registry->auto_approve = auto_approve;
    return CHARNESS_STATUS_OK;
}

void charness_tool_registry_deinit(charness_tool_registry_t *registry)
{
    if (registry == NULL) {
        return;
    }

    memset(registry, 0, sizeof(*registry));
}

charness_status_t charness_tool_registry_execute(charness_tool_registry_t *registry,
                                                 const charness_tool_request_t *request,
                                                 charness_tool_result_t *result_out)
{
    if (registry == NULL || request == NULL || result_out == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    result_out->text = NULL;
    result_out->exit_code = 0;

    char *details = describe_request(request);
    if (details == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    if (registry->logger != NULL) {
        charness_logger_logf(registry->logger,
                             CHARNESS_LOG_DEBUG,
                             "tools",
                             "request",
                             "%s",
                             details);
    }

    const bool needs_approval = !registry->auto_approve && request->requires_approval;
    if (needs_approval) {
        if (registry->approval_fn == NULL || !registry->approval_fn(registry->approval_user_data,
                                                                   charness_tool_kind_to_string(request->kind),
                                                                   details)) {
            result_out->text = charness_string_dup("tool execution denied by the approval gate\n");
            result_out->exit_code = 1;
            free(details);
            return CHARNESS_STATUS_NEEDS_APPROVAL;
        }
    }

    free(details);

    char *resolved_path = NULL;
    char *output = NULL;
    charness_status_t status = CHARNESS_STATUS_OK;

    switch (request->kind) {
    case CHARNESS_TOOL_READ_FILE:
        resolved_path = resolve_path(registry->workspace_root, request->path);
        if (resolved_path == NULL) {
            status = CHARNESS_STATUS_OUT_OF_MEMORY;
            break;
        }
        output = read_text_file(resolved_path);
        if (output == NULL) {
            status = CHARNESS_STATUS_IO_ERROR;
        }
        break;

    case CHARNESS_TOOL_WRITE_FILE: {
        resolved_path = resolve_path(registry->workspace_root, request->path);
        if (resolved_path == NULL) {
            status = CHARNESS_STATUS_OUT_OF_MEMORY;
            break;
        }

        char *old_text = read_text_file(resolved_path);
        FILE *file = fopen(resolved_path, "wb");
        if (file == NULL) {
            free(old_text);
            status = CHARNESS_STATUS_IO_ERROR;
            break;
        }

        const char *content = safe_text(request->content);
        const size_t content_length = strlen(content);
        const size_t written = fwrite(content, 1U, content_length, file);
        fclose(file);
        if (written != content_length) {
            free(old_text);
            status = CHARNESS_STATUS_IO_ERROR;
            break;
        }

        output = build_diff_preview(resolved_path, old_text, content);
        free(old_text);
        if (output == NULL) {
            status = CHARNESS_STATUS_OUT_OF_MEMORY;
        }
        break;
    }

    case CHARNESS_TOOL_RUN_COMMAND:
        result_out->text = charness_string_dup("run_command is intentionally stubbed in the skeleton; integrate a sandboxed executor before enabling it.\n");
        result_out->exit_code = 1;
        return CHARNESS_STATUS_UNAVAILABLE;

    case CHARNESS_TOOL_LIST_FILES:
        resolved_path = resolve_path(registry->workspace_root, request->path);
        if (resolved_path == NULL) {
            status = CHARNESS_STATUS_OUT_OF_MEMORY;
            break;
        }
        output = read_directory_listing(resolved_path);
        if (output == NULL) {
            status = CHARNESS_STATUS_IO_ERROR;
        }
        break;

    case CHARNESS_TOOL_GREP:
        resolved_path = resolve_path(registry->workspace_root, request->path);
        if (resolved_path == NULL) {
            status = CHARNESS_STATUS_OUT_OF_MEMORY;
            break;
        }
        output = recursive_grep(resolved_path, safe_text(request->pattern));
        if (output == NULL) {
            status = CHARNESS_STATUS_IO_ERROR;
        }
        break;

    case CHARNESS_TOOL_GIT_STATUS:
        output = git_status_output(registry->workspace_root);
        if (output == NULL) {
            status = CHARNESS_STATUS_IO_ERROR;
        }
        break;

    default:
        status = CHARNESS_STATUS_INVALID_ARGUMENT;
        break;
    }

    free(resolved_path);

    if (status != CHARNESS_STATUS_OK) {
        free(output);
        return status;
    }

    result_out->text = output;
    result_out->exit_code = 0;
    return CHARNESS_STATUS_OK;
}
