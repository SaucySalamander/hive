#ifndef CHARNESS_INFERENCE_ADAPTER_H
#define CHARNESS_INFERENCE_ADAPTER_H

#include "common/strings.h"
#include "core/types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct charness_logger;

/**
 * Input request sent to the inference adapter.
 */
typedef struct charness_inference_request {
    const char *agent_name;
    const char *system_prompt;
    const char *user_prompt;
    const char *context;
} charness_inference_request_t;

/**
 * Output produced by the inference adapter.
 */
typedef struct charness_inference_response {
    char *text;
    size_t estimated_tokens;
} charness_inference_response_t;

typedef struct charness_inference_adapter charness_inference_adapter_t;

/**
 * Inference adapter virtual table.
 */
typedef struct charness_inference_adapter_vtable {
    charness_status_t (*generate)(void *state,
                                  const charness_inference_request_t *request,
                                  charness_inference_response_t *response);
    void (*destroy)(void *state);
} charness_inference_adapter_vtable_t;

/**
 * Inference adapter instance.
 */
struct charness_inference_adapter {
    const charness_inference_adapter_vtable_t *vtable;
    void *state;
    bool is_mock;
};

/**
 * Initialize the built-in echo/mock inference adapter.
 *
 * @param adapter Adapter state to initialize.
 * @param logger Optional logger used by the mock adapter.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_inference_adapter_init_mock(charness_inference_adapter_t *adapter,
                                                       struct charness_logger *logger);

/**
 * Initialize an adapter from a custom vtable and state.
 *
 * @param adapter Adapter state to initialize.
 * @param vtable Function table implemented by the caller.
 * @param state Opaque custom state owned by the adapter.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_inference_adapter_init_custom(charness_inference_adapter_t *adapter,
                                                         const charness_inference_adapter_vtable_t *vtable,
                                                         void *state);

/**
 * Destroy an adapter instance.
 *
 * @param adapter Adapter state to destroy.
 */
void charness_inference_adapter_deinit(charness_inference_adapter_t *adapter);

/**
 * Generate text through the adapter.
 *
 * @param adapter Adapter state.
 * @param request Input request.
 * @param response Output response. The caller owns `response->text` on success.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_inference_adapter_generate(charness_inference_adapter_t *adapter,
                                                      const charness_inference_request_t *request,
                                                      charness_inference_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
