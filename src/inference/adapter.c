#include "inference/adapter.h"

#include "logging/logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct hive_mock_inference_state {
    struct hive_logger *logger;
} hive_mock_inference_state_t;

static const char *safe_text(const char *text)
{
    return text != NULL ? text : "";
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

    char *payload = hive_string_format(
        "[mock:%s]\n\n[system]\n%s\n\n[user]\n%s\n\n[context]\n%s\n",
        safe_text(request->agent_name),
        safe_text(request->system_prompt),
        safe_text(request->user_prompt),
        safe_text(request->context));

    if (payload == NULL) {
        return HIVE_STATUS_OUT_OF_MEMORY;
    }

    response->text = payload;
    response->estimated_tokens = estimate_tokens(payload);
    return HIVE_STATUS_OK;
}

static void mock_destroy(void *state)
{
    free(state);
}

static const hive_inference_adapter_vtable_t mock_vtable = {
    .generate = mock_generate,
    .destroy = mock_destroy,
};

hive_status_t hive_inference_adapter_init_mock(hive_inference_adapter_t *adapter,
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
    adapter->vtable = &mock_vtable;
    adapter->state = state;
    adapter->is_mock = true;
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
