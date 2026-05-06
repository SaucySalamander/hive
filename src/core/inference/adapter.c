#include "core/inference/adapter.h"

#include "core/inference/inference.h"

#include "common/logging/logger.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef HIVE_INFERENCE_TIMEOUT_MS
#  define HIVE_INFERENCE_TIMEOUT_MS 30000U
#endif

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

    /* Item #9: seed RNG lazily for jitter */
    static bool s_seeded = false;
    if (!s_seeded) {
        srand((unsigned int)time(NULL));
        s_seeded = true;
    }

    /* Item #10: read wall-clock timeout from env var once */
    static unsigned long s_timeout_ms;
    static bool s_timeout_init = false;
    if (!s_timeout_init) {
        s_timeout_ms = HIVE_INFERENCE_TIMEOUT_MS;
        const char *env_timeout = getenv("HIVE_INFERENCE_TIMEOUT_MS");
        if (env_timeout != NULL && *env_timeout != '\0') {
            char *end = NULL;
            unsigned long val = strtoul(env_timeout, &end, 10);
            if (end != env_timeout) {
                s_timeout_ms = val;
            }
        }
        s_timeout_init = true;
    }

    /* Items #9 + #10: retry loop with backoff and diagnostic timeout */
    char *payload = NULL;
    int result    = INF_ERR;

    for (int attempt = 0; attempt < 3; ++attempt) {
        /* Item #9: sleep before this attempt (0 ms for 0, 1000 ms for 1, 2000 ms for 2) */
        if (attempt > 0) {
            long base_ms  = (long)attempt * 1000L;
            long low_ms   = base_ms * 4L / 5L;
            long range_ms = base_ms * 2L / 5L;
            long sleep_ms = low_ms + (long)(rand() % (int)(range_ms + 1L));
            struct timespec sleep_ts = {
                .tv_sec  = sleep_ms / 1000L,
                .tv_nsec = (sleep_ms % 1000L) * 1000000L,
            };
            nanosleep(&sleep_ts, NULL);
        }

        /* Item #10: record wall-clock start */
        struct timespec t_start, t_end;
        clock_gettime(CLOCK_MONOTONIC, &t_start);

        payload = NULL;
        result  = inf_complete_sync(mock_state->ctx, &inf_request, &payload);

        /* Item #10: check diagnostic timeout after-the-fact */
        clock_gettime(CLOCK_MONOTONIC, &t_end);
        long elapsed_ms = (long)(t_end.tv_sec - t_start.tv_sec) * 1000L
                        + (long)(t_end.tv_nsec - t_start.tv_nsec) / 1000000L;
        if (s_timeout_ms != 0UL && elapsed_ms > 0L
                && (unsigned long)elapsed_ms > s_timeout_ms) {
            if (mock_state->logger != NULL) {
                hive_logger_logf(mock_state->logger,
                                 HIVE_LOG_WARN,
                                 "inference",
                                 "mock_generate_timeout",
                                 "inference call took %ldms, limit %lums",
                                 elapsed_ms, s_timeout_ms);
            }
            free(payload);
            return HIVE_STATUS_IO_ERROR;
        }

        if (result == INF_OK && payload != NULL) {
            break;
        }

        /* Item #9: log warn on non-final failure */
        if (attempt < 2) {
            if (mock_state->logger != NULL) {
                hive_logger_logf(mock_state->logger,
                                 HIVE_LOG_WARN,
                                 "inference",
                                 "mock_generate_retry",
                                 "attempt %d failed (%s), retrying",
                                 attempt,
                                 inf_last_error(mock_state->ctx));
            }
        }
    }

    /* All retries exhausted: handle failure */
    if (result != INF_OK || payload == NULL) {
        free(payload);

        /* Item #11: synthesise fallback on INF_ERR_BACKEND */
        if (result == INF_ERR_BACKEND) {
            if (mock_state->logger != NULL) {
                hive_logger_log(mock_state->logger,
                                HIVE_LOG_WARN,
                                "inference",
                                "mock_generate_fallback",
                                "backend unavailable after retries, using fallback response");
            }
            response->text = hive_string_dup("[FALLBACK: backend unavailable]");
            response->estimated_tokens = 5U;
            return HIVE_STATUS_OK;
        }

        /* Item #9: log error on final non-backend failure */
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

    /* Item #12: token budget enforcement (hard cap at 4096 tokens) */
    size_t tokens = estimate_tokens(payload);
    if (tokens > 4096U) {
        if (mock_state->logger != NULL) {
            hive_logger_logf(mock_state->logger,
                             HIVE_LOG_WARN,
                             "inference",
                             "mock_generate_truncated",
                             "response truncated from %zu to 4096 tokens",
                             tokens);
        }
        size_t word_count     = 0U;
        bool in_word          = false;
        unsigned char *cursor = (unsigned char *)payload;
        while (*cursor != '\0') {
            if (isspace(*cursor)) {
                in_word = false;
            } else if (!in_word) {
                in_word = true;
                ++word_count;
                if (word_count > 4096U) {
                    *cursor = '\0';
                    break;
                }
            }
            ++cursor;
        }
        tokens = 4096U;
    }

    response->text = payload;
    response->estimated_tokens = tokens;
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

    /* Check registration before attempting to create — gives a clear error */
    if (!inf_has_backend(backend_name)) {
        if (logger != NULL) {
            hive_logger_logf(logger, HIVE_LOG_ERROR,
                             "inference", "init_named",
                             "backend '%s' is not registered; "
                             "available: mock, copilotcli",
                             backend_name != NULL ? backend_name : "(null)");
        }
        free(state);
        return HIVE_STATUS_UNAVAILABLE;
    }

    state->ctx = inf_create(backend_name, config_json);
    if (state->ctx == NULL) {
        if (logger != NULL) {
            hive_logger_logf(logger, HIVE_LOG_ERROR,
                             "inference", "init_named",
                             "backend '%s' failed to initialise "
                             "(check binary in PATH, config JSON, and permissions)",
                             backend_name != NULL ? backend_name : "(null)");
        }
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
