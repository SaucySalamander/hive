#include "inference/adapter.h"

#include "logging/logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct charness_mock_inference_state {
    struct charness_logger *logger;
} charness_mock_inference_state_t;

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

static charness_status_t mock_generate(void *state,
                                       const charness_inference_request_t *request,
                                       charness_inference_response_t *response)
{
    if (state == NULL || request == NULL || response == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    const charness_mock_inference_state_t *mock_state = state;
    if (mock_state->logger != NULL) {
        charness_logger_logf(mock_state->logger,
                             CHARNESS_LOG_DEBUG,
                             "inference",
                             "mock_generate",
                             "echoing prompt for %s",
                             safe_text(request->agent_name));
    }

    char *payload = charness_string_format(
        "[mock:%s]\n\n[system]\n%s\n\n[user]\n%s\n\n[context]\n%s\n",
        safe_text(request->agent_name),
        safe_text(request->system_prompt),
        safe_text(request->user_prompt),
        safe_text(request->context));

    if (payload == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    response->text = payload;
    response->estimated_tokens = estimate_tokens(payload);
    return CHARNESS_STATUS_OK;
}

static void mock_destroy(void *state)
{
    free(state);
}

static const charness_inference_adapter_vtable_t mock_vtable = {
    .generate = mock_generate,
    .destroy = mock_destroy,
};

charness_status_t charness_inference_adapter_init_mock(charness_inference_adapter_t *adapter,
                                                       struct charness_logger *logger)
{
    if (adapter == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    charness_mock_inference_state_t *state = calloc(1U, sizeof(*state));
    if (state == NULL) {
        return CHARNESS_STATUS_OUT_OF_MEMORY;
    }

    state->logger = logger;
    adapter->vtable = &mock_vtable;
    adapter->state = state;
    adapter->is_mock = true;
    return CHARNESS_STATUS_OK;
}

charness_status_t charness_inference_adapter_init_custom(charness_inference_adapter_t *adapter,
                                                         const charness_inference_adapter_vtable_t *vtable,
                                                         void *state)
{
    if (adapter == NULL || vtable == NULL || vtable->generate == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    adapter->vtable = vtable;
    adapter->state = state;
    adapter->is_mock = false;
    return CHARNESS_STATUS_OK;
}

void charness_inference_adapter_deinit(charness_inference_adapter_t *adapter)
{
    if (adapter == NULL || adapter->vtable == NULL) {
        return;
    }

    if (adapter->vtable->destroy != NULL) {
        adapter->vtable->destroy(adapter->state);
    }

    memset(adapter, 0, sizeof(*adapter));
}

charness_status_t charness_inference_adapter_generate(charness_inference_adapter_t *adapter,
                                                      const charness_inference_request_t *request,
                                                      charness_inference_response_t *response)
{
    if (adapter == NULL || adapter->vtable == NULL || adapter->vtable->generate == NULL || response == NULL) {
        return CHARNESS_STATUS_INVALID_ARGUMENT;
    }

    response->text = NULL;
    response->estimated_tokens = 0U;
    return adapter->vtable->generate(adapter->state, request, response);
}
