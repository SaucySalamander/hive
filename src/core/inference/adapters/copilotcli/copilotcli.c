#include "copilotcli.h"

#include "common/strings.h"

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef struct copilotcli_stream {
    int cancel_token;
    pid_t child_pid;
    int stdout_fd;
    pthread_t reader_thread;
    inf_stream_cb_t callback;
    void *user_data;
    volatile int cancelled;
    struct copilotcli_stream *next;
} copilotcli_stream_t;

typedef struct copilotcli_state {
    char *cmd;
    char *error;
    pthread_mutex_t lock;
    copilotcli_stream_t *streams;
    int next_cancel_token;
} copilotcli_state_t;

static void set_error(copilotcli_state_t *state, const char *message)
{
    if (state == NULL) {
        return;
    }
    pthread_mutex_lock(&state->lock);
    free(state->error);
    state->error = hive_string_dup(message != NULL ? message : "");
    pthread_mutex_unlock(&state->lock);
}

static void *reader_thread_func(void *arg)
{
    copilotcli_stream_t *stream = arg;
    if (stream == NULL) {
        return NULL;
    }

    char buffer[4096];
    ssize_t nread;

    while ((nread = read(stream->stdout_fd, buffer, sizeof(buffer))) > 0) {
        if (!stream->cancelled && stream->callback != NULL) {
            stream->callback(buffer, (size_t)nread, stream->user_data);
        }
    }

    close(stream->stdout_fd);
    if (stream->child_pid > 0) {
        waitpid(stream->child_pid, NULL, 0);
    }

    return NULL;
}

static int copilotcli_init(const char *config_json, void **handle_out)
{
    (void)config_json;

    if (handle_out == NULL) {
        return INF_ERR;
    }

    copilotcli_state_t *state = calloc(1, sizeof(*state));
    if (state == NULL) {
        return INF_ERR_BACKEND;
    }

    if (pthread_mutex_init(&state->lock, NULL) != 0) {
        free(state);
        return INF_ERR_BACKEND;
    }

    state->next_cancel_token = 1;
    state->cmd = hive_string_dup("copilotcli");
    if (state->cmd == NULL) {
        free(state);
        return INF_ERR_BACKEND;
    }

    *handle_out = state;
    return INF_OK;
}

static void copilotcli_shutdown(void *handle)
{
    copilotcli_state_t *state = handle;
    if (state == NULL) {
        return;
    }

    pthread_mutex_lock(&state->lock);
    copilotcli_stream_t *stream = state->streams;
    state->streams = NULL;
    pthread_mutex_unlock(&state->lock);

    while (stream != NULL) {
        copilotcli_stream_t *next = stream->next;
        stream->cancelled = 1;
        if (stream->child_pid > 0) {
            kill(stream->child_pid, SIGTERM);
        }
        close(stream->stdout_fd);
        pthread_join(stream->reader_thread, NULL);
        free(stream);
        stream = next;
    }

    free(state->cmd);
    free(state->error);
    pthread_mutex_destroy(&state->lock);
    free(state);
}

static int copilotcli_complete_sync(void *handle, const inf_request_t *req,
                                     char **out_text)
{
    (void)handle;
    (void)req;
    (void)out_text;
    return INF_ERR_BACKEND;
}

static int copilotcli_complete_stream(void *handle, const inf_request_t *req,
                                       inf_stream_cb_t cb, void *user,
                                       int *cancel_token_out)
{
    copilotcli_state_t *state = handle;
    if (state == NULL || req == NULL || cb == NULL || cancel_token_out == NULL) {
        return INF_ERR;
    }

    int inpipe[2], outpipe[2];
    if (pipe(inpipe) < 0 || pipe(outpipe) < 0) {
        set_error(state, "Failed to create pipes");
        return INF_ERR_BACKEND;
    }

    pid_t pid = fork();
    if (pid < 0) {
        close(inpipe[0]);
        close(inpipe[1]);
        close(outpipe[0]);
        close(outpipe[1]);
        set_error(state, "Failed to fork");
        return INF_ERR_BACKEND;
    }

    if (pid == 0) {
        /* Child process */
        close(inpipe[1]);
        close(outpipe[0]);
        dup2(inpipe[0], STDIN_FILENO);
        dup2(outpipe[1], STDOUT_FILENO);
        close(inpipe[0]);
        close(outpipe[1]);

        execl(state->cmd, state->cmd, NULL);
        _exit(127);
    }

    /* Parent process */
    close(inpipe[0]);
    close(outpipe[1]);

    copilotcli_stream_t *stream = calloc(1, sizeof(*stream));
    if (stream == NULL) {
        close(inpipe[1]);
        close(outpipe[0]);
        kill(pid, SIGTERM);
        set_error(state, "Failed to allocate stream");
        return INF_ERR_BACKEND;
    }

    stream->child_pid = pid;
    stream->stdout_fd = outpipe[0];
    stream->callback = cb;
    stream->user_data = user;
    stream->cancelled = 0;

    pthread_mutex_lock(&state->lock);
    stream->cancel_token = state->next_cancel_token++;
    stream->next = state->streams;
    state->streams = stream;
    pthread_mutex_unlock(&state->lock);

    *cancel_token_out = stream->cancel_token;

    if (pthread_create(&stream->reader_thread, NULL, reader_thread_func, stream) != 0) {
        close(inpipe[1]);
        close(outpipe[0]);
        kill(pid, SIGTERM);
        pthread_mutex_lock(&state->lock);
        if (state->streams == stream) {
            state->streams = stream->next;
        }
        pthread_mutex_unlock(&state->lock);
        free(stream);
        set_error(state, "Failed to create reader thread");
        return INF_ERR_BACKEND;
    }

    close(inpipe[1]);
    return INF_OK;
}

static int copilotcli_list_models(void *handle, char ***models_out,
                                   size_t *count_out)
{
    copilotcli_state_t *state = handle;
    if (state == NULL || models_out == NULL || count_out == NULL) {
        return INF_ERR;
    }

    char **models = calloc(1, sizeof(char *));
    if (models == NULL) {
        return INF_ERR_BACKEND;
    }

    models[0] = hive_string_dup("default");
    if (models[0] == NULL) {
        free(models);
        return INF_ERR_BACKEND;
    }

    *models_out = models;
    *count_out = 1;
    return INF_OK;
}

static int copilotcli_cancel(void *handle, int cancel_token)
{
    copilotcli_state_t *state = handle;
    if (state == NULL) {
        return INF_ERR;
    }

    pthread_mutex_lock(&state->lock);
    copilotcli_stream_t *stream = state->streams;
    while (stream != NULL) {
        if (stream->cancel_token == cancel_token) {
            stream->cancelled = 1;
            if (stream->child_pid > 0) {
                kill(stream->child_pid, SIGINT);
            }
            pthread_mutex_unlock(&state->lock);
            return INF_OK;
        }
        stream = stream->next;
    }
    pthread_mutex_unlock(&state->lock);

    return INF_ERR;
}

static const char *copilotcli_last_error(void *handle)
{
    copilotcli_state_t *state = handle;
    if (state == NULL) {
        return "";
    }

    pthread_mutex_lock(&state->lock);
    const char *error = state->error != NULL ? state->error : "";
    pthread_mutex_unlock(&state->lock);

    return error;
}

const inf_adapter_vtable_t copilotcli_adapter_vtable = {
    .name = "copilotcli",
    .init = copilotcli_init,
    .shutdown = copilotcli_shutdown,
    .complete_sync = copilotcli_complete_sync,
    .complete_stream = copilotcli_complete_stream,
    .list_models = copilotcli_list_models,
    .cancel = copilotcli_cancel,
    .last_error = copilotcli_last_error,
};

int copilotcli_adapter_register(void)
{
    return inf_register_adapter(&copilotcli_adapter_vtable);
}
