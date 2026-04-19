#ifndef HIVE_CORE_INFERENCE_ADAPTER_H
#define HIVE_CORE_INFERENCE_ADAPTER_H

#include "common/strings.h"
#include "core/types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hive_logger;

typedef struct hive_inference_request {
    const char *agent_name;
    const char *system_prompt;
    const char *user_prompt;
    const char *context;
} hive_inference_request_t;

typedef struct hive_inference_response {
    char *text;
    size_t estimated_tokens;
} hive_inference_response_t;

typedef struct hive_inference_adapter hive_inference_adapter_t;

typedef struct hive_inference_adapter_vtable {
    hive_status_t (*generate)(void *state,
                                  const hive_inference_request_t *request,
                                  hive_inference_response_t *response);
    void (*destroy)(void *state);
} hive_inference_adapter_vtable_t;

struct hive_inference_adapter {
    const hive_inference_adapter_vtable_t *vtable;
    void *state;
    bool is_mock;
};

hive_status_t hive_inference_adapter_init_mock(hive_inference_adapter_t *adapter,
                                                       struct hive_logger *logger);

hive_status_t hive_inference_adapter_init_custom(hive_inference_adapter_t *adapter,
                                                         const hive_inference_adapter_vtable_t *vtable,
                                                         void *state);

void hive_inference_adapter_deinit(hive_inference_adapter_t *adapter);

hive_status_t hive_inference_adapter_init_named(hive_inference_adapter_t *adapter,
                                                        const char *backend_name,
                                                        const char *config_json,
                                                        struct hive_logger *logger);

hive_status_t hive_inference_adapter_generate(hive_inference_adapter_t *adapter,
                                                      const hive_inference_request_t *request,
                                                      hive_inference_response_t *response);

#ifdef __cplusplus
}
#endif

#endif
