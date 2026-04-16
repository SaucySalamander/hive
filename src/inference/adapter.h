#ifndef HIVE_INFERENCE_ADAPTER_H
#define HIVE_INFERENCE_ADAPTER_H

#include "common/strings.h"
#include "core/types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hive_logger;

/**
 * Input request sent to the inference adapter.
 */
typedef struct hive_inference_request {
    const char *agent_name;
    const char *system_prompt;
    const char *user_prompt;
    const char *context;
} hive_inference_request_t;

/**
 * Output produced by the inference adapter.
 */
typedef struct hive_inference_response {
    char *text;
    size_t estimated_tokens;
} hive_inference_response_t;

typedef struct hive_inference_adapter hive_inference_adapter_t;

/**
 * Inference adapter virtual table.
 */
typedef struct hive_inference_adapter_vtable {
    hive_status_t (*generate)(void *state,
                                  const hive_inference_request_t *request,
                                  hive_inference_response_t *response);
    void (*destroy)(void *state);
} hive_inference_adapter_vtable_t;

/**
 * Inference adapter instance.
 */
struct hive_inference_adapter {
    const hive_inference_adapter_vtable_t *vtable;
    void *state;
    bool is_mock;
};

/**
 * Initialize the built-in echo/mock inference adapter.
 *
 * @param adapter Adapter state to initialize.
 * @param logger Optional logger used by the mock adapter.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_inference_adapter_init_mock(hive_inference_adapter_t *adapter,
                                                       struct hive_logger *logger);

/**
 * Initialize an adapter from a custom vtable and state.
 *
 * @param adapter Adapter state to initialize.
 * @param vtable Function table implemented by the caller.
 * @param state Opaque custom state owned by the adapter.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_inference_adapter_init_custom(hive_inference_adapter_t *adapter,
                                                         const hive_inference_adapter_vtable_t *vtable,
                                                         void *state);

/**
 * Destroy an adapter instance.
 *
 * @param adapter Adapter state to destroy.
 */
void hive_inference_adapter_deinit(hive_inference_adapter_t *adapter);

/**
 * Generate text through the adapter.
 *
 * @param adapter Adapter state.
 * @param request Input request.
 * @param response Output response. The caller owns `response->text` on success.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_inference_adapter_generate(hive_inference_adapter_t *adapter,
                                                      const hive_inference_request_t *request,
                                                      hive_inference_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
