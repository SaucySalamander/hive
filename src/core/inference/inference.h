#ifndef HIVE_CORE_INFERENCE_INFERENCE_H
#define HIVE_CORE_INFERENCE_INFERENCE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct inf_ctx inf_ctx_t;

typedef enum inf_result {
    INF_OK = 0,
    INF_ERR = -1,
    INF_ERR_TIMEOUT = -2,
    INF_ERR_CANCELLED = -3,
    INF_ERR_BACKEND = -4
} inf_result_t;

typedef struct inf_message {
    const char *role;
    const char *content;
} inf_message_t;

typedef struct inf_request {
    const char *model;
    const inf_message_t *messages;
    size_t n_messages;
    double temperature;
    int max_tokens;
    const char *user;
    int stream;
} inf_request_t;

typedef void (*inf_stream_cb_t)(const char *chunk, size_t len, void *user);

typedef struct inf_adapter_vtable {
    const char *name;
    int (*init)(const char *config_json, void **handle_out);
    void (*shutdown)(void *handle);
    int (*complete_sync)(void *handle, const inf_request_t *req, char **out_text);
    int (*complete_stream)(void *handle,
                           const inf_request_t *req,
                           inf_stream_cb_t cb,
                           void *user,
                           int *cancel_token_out);
    int (*list_models)(void *handle, char ***models_out, size_t *count_out);
    int (*cancel)(void *handle, int cancel_token);
    const char *(*last_error)(void *handle);
} inf_adapter_vtable_t;

int inf_register_adapter(const void *adapter_vtable);
int inf_has_backend(const char *backend_name);
inf_ctx_t *inf_create(const char *backend_name, const char *config_json);
void inf_destroy(inf_ctx_t *ctx);

int inf_list_models(inf_ctx_t *ctx, char ***models_out, size_t *count_out);
void inf_free_model_list(char **models, size_t count);

int inf_complete_sync(inf_ctx_t *ctx, const inf_request_t *req, char **out_text);
int inf_complete_stream(inf_ctx_t *ctx,
                        const inf_request_t *req,
                        inf_stream_cb_t cb,
                        void *user,
                        int *cancel_token_out);
int inf_cancel(inf_ctx_t *ctx, int cancel_token);

void inf_free(char *ptr);
const char *inf_last_error(inf_ctx_t *ctx);

#ifdef __cplusplus
}
#endif

#endif
