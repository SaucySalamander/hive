#include "core/inference/inference.h"

#include "common/strings.h"

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

typedef struct inf_stream_entry {
    int token;
    atomic_bool cancelled;
    struct inf_stream_entry *next;
} inf_stream_entry_t;

typedef struct inf_registry_entry {
    char *name;
    const inf_adapter_vtable_t *vtable;
} inf_registry_entry_t;

typedef struct inf_mock_state {
    pthread_mutex_t lock;
    inf_stream_entry_t *streams;
    int next_token;
    char *last_error;
} inf_mock_state_t;

struct inf_ctx {
    const inf_adapter_vtable_t *vtable;
    void *handle;
    char *last_error;
    pthread_mutex_t lock;
    inf_stream_entry_t *fallback_streams;
    int next_cancel_token;
};

static pthread_mutex_t g_registry_lock = PTHREAD_MUTEX_INITIALIZER;
static inf_registry_entry_t *g_registry = NULL;
static size_t g_registry_count = 0U;
static size_t g_registry_capacity = 0U;
static bool g_registry_ready = false;
static bool g_registry_cleanup_registered = false;

static const inf_adapter_vtable_t mock_vtable;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static void ctx_set_error(inf_ctx_t *ctx, const char *message)
{
    if (ctx == NULL) {
        return;
    }

    char *copy = hive_string_dup(safe_text(message));
    if (copy == NULL) {
        return;
    }

    pthread_mutex_lock(&ctx->lock);
    free(ctx->last_error);
    ctx->last_error = copy;
    pthread_mutex_unlock(&ctx->lock);
}

static void mock_set_error(inf_mock_state_t *state, const char *message)
{
    if (state == NULL) {
        return;
    }

    char *copy = hive_string_dup(safe_text(message));
    if (copy == NULL) {
        return;
    }

    pthread_mutex_lock(&state->lock);
    free(state->last_error);
    state->last_error = copy;
    pthread_mutex_unlock(&state->lock);
}

static const char *mock_find_message(const inf_request_t *req, const char *role)
{
    if (req == NULL || role == NULL) {
        return "";
    }

    for (size_t index = 0U; index < req->n_messages; ++index) {
        const inf_message_t *message = &req->messages[index];
        if (message->role != NULL && strcmp(message->role, role) == 0) {
            return safe_text(message->content);
        }
    }

    if (strcmp(role, "user") == 0) {
        return safe_text(req->user);
    }

    return "";
}

static int registry_reserve_locked(size_t needed)
{
    if (needed <= g_registry_capacity) {
        return INF_OK;
    }

    size_t new_capacity = g_registry_capacity == 0U ? 4U : g_registry_capacity;
    while (new_capacity < needed) {
        new_capacity *= 2U;
    }

    inf_registry_entry_t *entries = realloc(g_registry, new_capacity * sizeof(*entries));
    if (entries == NULL) {
        return INF_ERR_BACKEND;
    }

    g_registry = entries;
    g_registry_capacity = new_capacity;
    return INF_OK;
}

static int registry_add_locked(const inf_adapter_vtable_t *vtable)
{
    if (vtable == NULL || vtable->name == NULL || vtable->name[0] == '\0') {
        return INF_ERR_BACKEND;
    }

    if (vtable->init == NULL || vtable->shutdown == NULL ||
        vtable->complete_sync == NULL || vtable->list_models == NULL) {
        return INF_ERR_BACKEND;
    }

    if (vtable->complete_stream != NULL && vtable->cancel == NULL) {
        return INF_ERR_BACKEND;
    }

    for (size_t index = 0U; index < g_registry_count; ++index) {
        if (strcmp(g_registry[index].name, vtable->name) == 0) {
            return INF_ERR_BACKEND;
        }
    }

    const int reserve_status = registry_reserve_locked(g_registry_count + 1U);
    if (reserve_status != INF_OK) {
        return reserve_status;
    }

    char *name_copy = hive_string_dup(vtable->name);
    if (name_copy == NULL) {
        return INF_ERR_BACKEND;
    }

    g_registry[g_registry_count].name = name_copy;
    g_registry[g_registry_count].vtable = vtable;
    ++g_registry_count;
    return INF_OK;
}

static const inf_adapter_vtable_t *registry_find_locked(const char *name)
{
    if (name == NULL || name[0] == '\0') {
        name = "mock";
    }

    for (size_t index = 0U; index < g_registry_count; ++index) {
        if (strcmp(g_registry[index].name, name) == 0) {
            return g_registry[index].vtable;
        }
    }

    return NULL;
}

static void registry_cleanup(void)
{
    pthread_mutex_lock(&g_registry_lock);
    for (size_t index = 0U; index < g_registry_count; ++index) {
        free(g_registry[index].name);
    }
    free(g_registry);
    g_registry = NULL;
    g_registry_count = 0U;
    g_registry_capacity = 0U;
    g_registry_ready = false;
    pthread_mutex_unlock(&g_registry_lock);
}

static void registry_ensure_ready(void)
{
    pthread_mutex_lock(&g_registry_lock);
    if (!g_registry_ready) {
        (void)registry_add_locked(&mock_vtable);
        if (!g_registry_cleanup_registered) {
            g_registry_cleanup_registered = true;
            atexit(registry_cleanup);
        }
        g_registry_ready = true;
    }
    pthread_mutex_unlock(&g_registry_lock);
}

static inf_stream_entry_t *stream_add_locked(inf_stream_entry_t **head,
                                             int *next_token,
                                             int *token_out)
{
    inf_stream_entry_t *entry = calloc(1U, sizeof(*entry));
    if (entry == NULL) {
        return NULL;
    }

    entry->token = (*next_token)++;
    atomic_store(&entry->cancelled, false);
    entry->next = *head;
    *head = entry;

    if (token_out != NULL) {
        *token_out = entry->token;
    }

    return entry;
}

static void stream_remove_locked(inf_stream_entry_t **head, int token)
{
    inf_stream_entry_t **cursor = head;
    while (*cursor != NULL) {
        if ((*cursor)->token == token) {
            inf_stream_entry_t *entry = *cursor;
            *cursor = entry->next;
            free(entry);
            return;
        }
        cursor = &(*cursor)->next;
    }
}

static inf_stream_entry_t *stream_find_locked(inf_stream_entry_t *head, int token)
{
    for (inf_stream_entry_t *cursor = head; cursor != NULL; cursor = cursor->next) {
        if (cursor->token == token) {
            return cursor;
        }
    }

    return NULL;
}

static char *mock_build_response(const inf_request_t *req)
{
    return hive_string_format(
        "[mock:%s]\n\n[system]\n%s\n\n[user]\n%s\n\n[context]\n%s\n",
        safe_text(req != NULL ? req->model : NULL),
        mock_find_message(req, "system"),
        mock_find_message(req, "user"),
        mock_find_message(req, "context"));
}

static int mock_init(const char *config_json, void **handle_out)
{
    (void)config_json;

    if (handle_out == NULL) {
        return INF_ERR;
    }

    inf_mock_state_t *state = calloc(1U, sizeof(*state));
    if (state == NULL) {
        return INF_ERR_BACKEND;
    }

    if (pthread_mutex_init(&state->lock, NULL) != 0) {
        free(state);
        return INF_ERR_BACKEND;
    }

    state->next_token = 1;
    *handle_out = state;
    return INF_OK;
}

static void mock_shutdown(void *handle)
{
    inf_mock_state_t *state = handle;
    if (state == NULL) {
        return;
    }

    pthread_mutex_lock(&state->lock);
    inf_stream_entry_t *cursor = state->streams;
    state->streams = NULL;
    pthread_mutex_unlock(&state->lock);

    while (cursor != NULL) {
        inf_stream_entry_t *next = cursor->next;
        free(cursor);
        cursor = next;
    }

    pthread_mutex_destroy(&state->lock);
    free(state->last_error);
    free(state);
}

static int mock_complete_sync(void *handle, const inf_request_t *req, char **out_text)
{
    inf_mock_state_t *state = handle;
    if (state == NULL || req == NULL || out_text == NULL) {
        return INF_ERR;
    }

    *out_text = NULL;
    char *text = mock_build_response(req);
    if (text == NULL) {
        mock_set_error(state, "mock backend could not allocate response text");
        return INF_ERR_BACKEND;
    }

    *out_text = text;
    return INF_OK;
}

static int mock_complete_stream(void *handle,
                                const inf_request_t *req,
                                inf_stream_cb_t cb,
                                void *user,
                                int *cancel_token_out)
{
    inf_mock_state_t *state = handle;
    if (state == NULL || req == NULL || cb == NULL) {
        return INF_ERR;
    }

    pthread_mutex_lock(&state->lock);
    inf_stream_entry_t *entry = stream_add_locked(&state->streams, &state->next_token, cancel_token_out);
    pthread_mutex_unlock(&state->lock);
    if (entry == NULL) {
        mock_set_error(state, "mock backend could not create a stream token");
        return INF_ERR_BACKEND;
    }

    char *text = NULL;
    int status = mock_complete_sync(handle, req, &text);
    if (status != INF_OK) {
        pthread_mutex_lock(&state->lock);
        stream_remove_locked(&state->streams, entry->token);
        pthread_mutex_unlock(&state->lock);
        return status;
    }

    const size_t text_length = strlen(text);
    const size_t chunk_size = 48U;
    for (size_t offset = 0U; offset < text_length; offset += chunk_size) {
        if (atomic_load(&entry->cancelled)) {
            free(text);
            pthread_mutex_lock(&state->lock);
            stream_remove_locked(&state->streams, entry->token);
            pthread_mutex_unlock(&state->lock);
            mock_set_error(state, "mock stream cancelled");
            return INF_ERR_CANCELLED;
        }

        const size_t remaining = text_length - offset;
        const size_t len = remaining < chunk_size ? remaining : chunk_size;
        cb(text + offset, len, user);
    }

    free(text);
    pthread_mutex_lock(&state->lock);
    stream_remove_locked(&state->streams, entry->token);
    pthread_mutex_unlock(&state->lock);
    return INF_OK;
}

static int mock_list_models(void *handle, char ***models_out, size_t *count_out)
{
    inf_mock_state_t *state = handle;
    if (state == NULL || models_out == NULL || count_out == NULL) {
        return INF_ERR;
    }

    *models_out = NULL;
    *count_out = 0U;

    char **models = calloc(2U, sizeof(*models));
    if (models == NULL) {
        mock_set_error(state, "mock backend could not allocate model list");
        return INF_ERR_BACKEND;
    }

    models[0] = hive_string_dup("hive:mock");
    models[1] = hive_string_dup("hive:echo");
    if (models[0] == NULL || models[1] == NULL) {
        free(models[0]);
        free(models[1]);
        free(models);
        mock_set_error(state, "mock backend could not allocate model names");
        return INF_ERR_BACKEND;
    }

    *models_out = models;
    *count_out = 2U;
    return INF_OK;
}

static int mock_cancel(void *handle, int token)
{
    inf_mock_state_t *state = handle;
    if (state == NULL) {
        return INF_ERR;
    }

    pthread_mutex_lock(&state->lock);
    inf_stream_entry_t *entry = stream_find_locked(state->streams, token);
    if (entry != NULL) {
        atomic_store(&entry->cancelled, true);
    }
    pthread_mutex_unlock(&state->lock);
    return INF_OK;
}

static const char *mock_last_error(void *handle)
{
    inf_mock_state_t *state = handle;
    if (state == NULL) {
        return "";
    }

    const char *out = "";
    pthread_mutex_lock(&state->lock);
    out = safe_text(state->last_error);
    pthread_mutex_unlock(&state->lock);
    return out;
}

static const inf_adapter_vtable_t mock_vtable = {
    .name = "mock",
    .init = mock_init,
    .shutdown = mock_shutdown,
    .complete_sync = mock_complete_sync,
    .complete_stream = mock_complete_stream,
    .list_models = mock_list_models,
    .cancel = mock_cancel,
    .last_error = mock_last_error,
};

int inf_register_adapter(const void *adapter_vtable)
{
    if (adapter_vtable == NULL) {
        return INF_ERR;
    }

    pthread_mutex_lock(&g_registry_lock);
    const int status = registry_add_locked((const inf_adapter_vtable_t *)adapter_vtable);
    pthread_mutex_unlock(&g_registry_lock);
    return status;
}

inf_ctx_t *inf_create(const char *backend_name, const char *config_json)
{
    registry_ensure_ready();

    pthread_mutex_lock(&g_registry_lock);
    const inf_adapter_vtable_t *vtable = registry_find_locked(backend_name);
    if (vtable == NULL) {
        pthread_mutex_unlock(&g_registry_lock);
        return NULL;
    }

    inf_ctx_t *ctx = calloc(1U, sizeof(*ctx));
    if (ctx == NULL) {
        pthread_mutex_unlock(&g_registry_lock);
        return NULL;
    }

    ctx->vtable = vtable;
    pthread_mutex_unlock(&g_registry_lock);

    if (vtable->init != NULL) {
        void *handle = NULL;
        if (vtable->init(config_json, &handle) != INF_OK) {
            free(ctx);
            return NULL;
        }
        ctx->handle = handle;
    }

    if (pthread_mutex_init(&ctx->lock, NULL) != 0) {
        if (ctx->vtable->shutdown != NULL && ctx->handle != NULL) {
            ctx->vtable->shutdown(ctx->handle);
        }
        free(ctx);
        return NULL;
    }

    ctx->last_error = NULL;
    ctx->fallback_streams = NULL;
    ctx->next_cancel_token = 1;
    return ctx;
}

void inf_destroy(inf_ctx_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    if (ctx->vtable != NULL && ctx->vtable->shutdown != NULL && ctx->handle != NULL) {
        ctx->vtable->shutdown(ctx->handle);
    }

    pthread_mutex_lock(&ctx->lock);
    inf_stream_entry_t *cursor = ctx->fallback_streams;
    ctx->fallback_streams = NULL;
    pthread_mutex_unlock(&ctx->lock);

    while (cursor != NULL) {
        inf_stream_entry_t *next = cursor->next;
        free(cursor);
        cursor = next;
    }

    free(ctx->last_error);
    pthread_mutex_destroy(&ctx->lock);
    free(ctx);
}

int inf_list_models(inf_ctx_t *ctx, char ***models_out, size_t *count_out)
{
    if (ctx == NULL || ctx->vtable == NULL || ctx->vtable->list_models == NULL) {
        return INF_ERR;
    }

    return ctx->vtable->list_models(ctx->handle, models_out, count_out);
}

void inf_free_model_list(char **models, size_t count)
{
    if (models == NULL) return;
    for (size_t i = 0; i < count; ++i) free(models[i]);
    free(models);
}

int inf_complete_sync(inf_ctx_t *ctx, const inf_request_t *req, char **out_text)
{
    if (ctx == NULL || ctx->vtable == NULL || ctx->vtable->complete_sync == NULL) {
        return INF_ERR;
    }

    ctx_set_error(ctx, NULL);
    const int res = ctx->vtable->complete_sync(ctx->handle, req, out_text);
    if (res != INF_OK) {
        if (ctx->vtable->last_error != NULL) {
            ctx_set_error(ctx, ctx->vtable->last_error(ctx->handle));
        }
    }
    return res;
}

int inf_complete_stream(inf_ctx_t *ctx,
                        const inf_request_t *req,
                        inf_stream_cb_t cb,
                        void *user,
                        int *cancel_token_out)
{
    if (ctx == NULL || ctx->vtable == NULL || ctx->vtable->complete_stream == NULL) {
        return INF_ERR;
    }

    ctx_set_error(ctx, NULL);
    const int res = ctx->vtable->complete_stream(ctx->handle, req, cb, user, cancel_token_out);
    if (res != INF_OK) {
        if (ctx->vtable->last_error != NULL) {
            ctx_set_error(ctx, ctx->vtable->last_error(ctx->handle));
        }
    }
    return res;
}

int inf_cancel(inf_ctx_t *ctx, int cancel_token)
{
    if (ctx == NULL || ctx->vtable == NULL || ctx->vtable->cancel == NULL) {
        return INF_ERR;
    }

    return ctx->vtable->cancel(ctx->handle, cancel_token);
}

void inf_free(char *ptr)
{
    free(ptr);
}

const char *inf_last_error(inf_ctx_t *ctx)
{
    if (ctx == NULL) return "";
    const char *out = "";
    pthread_mutex_lock(&ctx->lock);
    out = safe_text(ctx->last_error);
    pthread_mutex_unlock(&ctx->lock);
    return out;
}
