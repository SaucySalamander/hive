#include "inference/adapter.h"

#include "inference/inference.h"

#include "logging/logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct hive_mock_inference_state {
    struct hive_logger *logger;
    inf_ctx_t *ctx;
} hive_mock_inference_state_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
}

static hive_status_t map_inf_result(int result)
{
    switch (result) {
    case INF_OK:
        return HIVE_STATUS_OK;
    case INF_ERR_CANCELLED:
        return HIVE_STATUS_CANCELLED;
    case INF_ERR_TIMEOUT:
        return HIVE_STATUS_IO_ERROR;
    case INF_ERR_BACKEND:
        return HIVE_STATUS_UNAVAILABLE;
    case INF_ERR:
    default:
        return HIVE_STATUS_ERROR;
    }
}

static size_t estimate_tokens(const char *text)
{
    size_t tokens = 0U;
    bool in_word = false;

    for (const unsigned char *cursor = (const unsigned char *)safe_text(text); *cursor != '\0'; ++cursor) {
        if (isspace(*cursor)) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++tokens;
        }
    }

    return tokens;
}

static hive_status_t mock_generate(void *state,
                                       const hive_inference_request_t *request,
                                       hive_inference_response_t *response)
{
    if (state == NULL || request == NULL || response == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    const hive_mock_inference_state_t *mock_state = state;
    if (mock_state->logger != NULL) {
        hive_logger_logf(mock_state->logger,
                             HIVE_LOG_DEBUG,
                             "inference",
                             "mock_generate",
                             "echoing prompt for %s",
                             safe_text(request->agent_name));
    }

    inf_message_t messages[3] = {
        { .role = "system", .content = safe_text(request->system_prompt) },
        { .role = "user", .content = safe_text(request->user_prompt) },
        { .role = "context", .content = safe_text(request->context) },
    };

    inf_request_t inf_request = {
        .model = safe_text(request->agent_name),
        .messages = messages,
        .n_messages = 3U,
        .temperature = 0.0,
        .max_tokens = 0,
        .user = safe_text(request->agent_name),
        .stream = 0,
    };

    char *payload = NULL;
    const int result = inf_complete_sync(mock_state->ctx, &inf_request, &payload);
    if (result != INF_OK || payload == NULL) {
        if (mock_state->logger != NULL) {
            hive_logger_logf(mock_state->logger,
                                 HIVE_LOG_ERROR,
                                 "inference",
                                 "mock_generate_failed",
                                 "%s",
                                 inf_last_error(mock_state->ctx));
        }
        return map_inf_result(result);
    }

    response->text = payload;
    response->estimated_tokens = estimate_tokens(payload);
    return HIVE_STATUS_OK;
}

static void mock_destroy(void *state)
{
    hive_mock_inference_state_t *mock_state = state;
    if (mock_state == NULL) {
        return;
    }

    inf_destroy(mock_state->ctx);
    free(mock_state);
}

static const hive_inference_adapter_vtable_t mock_vtable = {
    .generate = mock_generate,
    .destroy = mock_destroy,
};

hive_status_t hive_inference_adapter_init_mock(hive_inference_adapter_t *adapter,
                                                       struct hive_logger *logger)
{
    return hive_inference_adapter_init_named(adapter, "mock", NULL, logger);
}

hive_status_t hive_inference_adapter_init_named(hive_inference_adapter_t *adapter,
                                                        const char *backend_name,
                                                        const char *config_json,
                                                        struct hive_logger *logger)
{
    if (adapter == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    hive_mock_inference_state_t *state = calloc(1U, sizeof(*state));
    if (state == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    state->logger = logger;
    state->ctx = inf_create(backend_name, config_json);
    if (state->ctx == NULL) {
        free(state);
        return HIVE_STATUS_UNAVAILABLE;
    }

    adapter->vtable = &mock_vtable;
    adapter->state = state;
    adapter->is_mock = backend_name == NULL || strcmp(backend_name, "mock") == 0;
    return HIVE_STATUS_OK;
}

hive_status_t hive_inference_adapter_init_custom(hive_inference_adapter_t *adapter,
                                                         const hive_inference_adapter_vtable_t *vtable,
                                                         void *state)
{
    if (adapter == NULL || vtable == NULL || vtable->generate == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    adapter->vtable = vtable;
    adapter->state = state;
    adapter->is_mock = false;
    return HIVE_STATUS_OK;
}

void hive_inference_adapter_deinit(hive_inference_adapter_t *adapter)
{
    if (adapter == NULL || adapter->vtable == NULL) {
        return;
    }

    if (adapter->vtable->destroy != NULL) {
        adapter->vtable->destroy(adapter->state);
    }

    memset(adapter, 0, sizeof(*adapter));
}

hive_status_t hive_inference_adapter_generate(hive_inference_adapter_t *adapter,
                                                      const hive_inference_request_t *request,
                                                      hive_inference_response_t *response)
{
    if (adapter == NULL || adapter->vtable == NULL || adapter->vtable->generate == NULL || response == NULL) {
        return HIVE_STATUS_INVALID_ARGUMENT;
    }

    response->text = NULL;
    response->estimated_tokens = 0U;
    return adapter->vtable->generate(adapter->state, request, response);
}
